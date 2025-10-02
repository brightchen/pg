package bc.monitor;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import org.apache.flink.api.common.eventtime.WatermarkStrategy;
import org.apache.flink.api.common.serialization.SimpleStringSchema;
import org.apache.flink.api.common.state.ValueState;
import org.apache.flink.api.common.state.ValueStateDescriptor;
import org.apache.flink.api.common.typeinfo.TypeInformation;
import org.apache.flink.configuration.Configuration;
import org.apache.flink.connector.kafka.sink.KafkaRecordSerializationSchema;
import org.apache.flink.connector.kafka.sink.KafkaSink;
import org.apache.flink.connector.kafka.source.KafkaSource;
import org.apache.flink.connector.kafka.source.enumerator.initializer.OffsetsInitializer;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.streaming.api.functions.KeyedProcessFunction;
import org.apache.flink.streaming.api.functions.windowing.ProcessAllWindowFunction;
import org.apache.flink.streaming.api.windowing.assigners.TumblingEventTimeWindows;
import org.apache.flink.streaming.api.windowing.time.Time;
import org.apache.flink.streaming.api.windowing.windows.TimeWindow;
import org.apache.flink.util.Collector;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;

/**
 * Flink job to monitor IoT device events and generate alerts for inactive devices.
 *
 * Reads events from Kafka topic "events", tracks last event time per device,
 * and generates alerts every 30 seconds for devices that haven't sent an event
 * for more than 1 minute.
 */
public class Monitor {

    private static final Logger LOG = LoggerFactory.getLogger(Monitor.class);
    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper()
            .disable(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS);

    private static final long INACTIVITY_THRESHOLD_MS = 60_000; // 1 minute
    private static final long WINDOW_SIZE_MS = 30_000; // 30 seconds

    public static void main(String[] args) throws Exception {
        // Parse arguments
        String kafkaBootstrapServers = getParameterOrDefault(args, "kafka.bootstrap.servers", "localhost:9092");
        String eventsTopicName = getParameterOrDefault(args, "events.topic", "events");
        String alertsTopicName = getParameterOrDefault(args, "alerts.topic", "alerts");
        String checkpointDir = getParameterOrDefault(args, "checkpoint.dir", "file:///tmp/flink-checkpoints");
        int parallelism = Integer.parseInt(getParameterOrDefault(args, "parallelism", "2"));

        LOG.info("Starting Monitor with parameters:");
        LOG.info("  kafka.bootstrap.servers: {}", kafkaBootstrapServers);
        LOG.info("  events.topic: {}", eventsTopicName);
        LOG.info("  alerts.topic: {}", alertsTopicName);
        LOG.info("  checkpoint.dir: {}", checkpointDir);
        LOG.info("  parallelism: {}", parallelism);

        // Set up the execution environment
        StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
        env.setParallelism(parallelism);

        // Configure checkpointing
        env.enableCheckpointing(10_000); // checkpoint every 10 seconds
        env.getCheckpointConfig().setCheckpointStorage(checkpointDir);
        env.getCheckpointConfig().setMinPauseBetweenCheckpoints(5_000);
        env.getCheckpointConfig().setCheckpointTimeout(60_000);

        // Create Kafka source
        KafkaSource<String> kafkaSource = KafkaSource.<String>builder()
                .setBootstrapServers(kafkaBootstrapServers)
                .setTopics(eventsTopicName)
                .setGroupId("monitor-group")
                .setStartingOffsets(OffsetsInitializer.latest())
                .setValueOnlyDeserializer(new SimpleStringSchema())
                .build();

        // Create data stream from Kafka
        DataStream<String> eventStream = env.fromSource(
                kafkaSource,
                WatermarkStrategy.<String>forBoundedOutOfOrderness(Duration.ofSeconds(10))
                        .withTimestampAssigner((event, timestamp) -> {
                            try {
                                DeviceEvent deviceEvent = OBJECT_MAPPER.readValue(event, DeviceEvent.class);
                                return deviceEvent.getTimestamp();
                            } catch (Exception e) {
                                LOG.error("Error parsing event for timestamp: {}", event, e);
                                return System.currentTimeMillis();
                            }
                        }),
                "Kafka Events Source"
        );

        // Parse events
        DataStream<DeviceEvent> deviceEvents = eventStream
                .map(json -> {
                    try {
                        return OBJECT_MAPPER.readValue(json, DeviceEvent.class);
                    } catch (Exception e) {
                        LOG.error("Error parsing event: {}", json, e);
                        return null;
                    }
                })
                .filter(event -> event != null)
                .returns(TypeInformation.of(DeviceEvent.class));

        // Track device state and generate alerts
        DataStream<DeviceAlert> alerts = deviceEvents
                .keyBy(DeviceEvent::getDeviceId)
                .process(new DeviceStateTracker())
                .windowAll(TumblingEventTimeWindows.of(Time.milliseconds(WINDOW_SIZE_MS)))
                .process(new AlertAggregator());

        // Create Kafka sink
        KafkaSink<String> kafkaSink = KafkaSink.<String>builder()
                .setBootstrapServers(kafkaBootstrapServers)
                .setRecordSerializer(KafkaRecordSerializationSchema.builder()
                        .setTopic(alertsTopicName)
                        .setValueSerializationSchema(new SimpleStringSchema())
                        .build()
                )
                .build();

        // Convert alerts to JSON and write to Kafka
        alerts
                .map(alert -> {
                    try {
                        return OBJECT_MAPPER.writeValueAsString(alert);
                    } catch (Exception e) {
                        LOG.error("Error serializing alert: {}", alert, e);
                        return null;
                    }
                })
                .filter(json -> json != null)
                .sinkTo(kafkaSink);

        // Execute the job
        env.execute("IoT Device Monitor");
    }

    /**
     * Tracks the last event time for each device and generates alerts for inactive devices.
     */
    public static class DeviceStateTracker extends KeyedProcessFunction<String, DeviceEvent, DeviceAlert> {

        private ValueState<Long> lastEventTimeState;
        private ValueState<Long> nextCheckTimeState;

        @Override
        public void open(Configuration parameters) {
            lastEventTimeState = getRuntimeContext().getState(
                    new ValueStateDescriptor<>("lastEventTime", Long.class));
            nextCheckTimeState = getRuntimeContext().getState(
                    new ValueStateDescriptor<>("nextCheckTime", Long.class));
        }

        @Override
        public void processElement(DeviceEvent event, Context ctx, Collector<DeviceAlert> out) throws Exception {
            String deviceId = event.getDeviceId();
            long eventTime = event.getTimestamp();
            Long lastEventTime = lastEventTimeState.value();

            // Update last event time if this is a newer event
            if (lastEventTime == null || eventTime > lastEventTime) {
                lastEventTimeState.update(eventTime);

                // Register timer for inactivity check
                long checkTime = eventTime + INACTIVITY_THRESHOLD_MS;
                Long currentNextCheckTime = nextCheckTimeState.value();

                if (currentNextCheckTime == null || checkTime < currentNextCheckTime) {
                    if (currentNextCheckTime != null) {
                        ctx.timerService().deleteEventTimeTimer(currentNextCheckTime);
                    }
                    ctx.timerService().registerEventTimeTimer(checkTime);
                    nextCheckTimeState.update(checkTime);
                }

                LOG.debug("Updated device {} last event time to {}, next check at {}",
                        deviceId, eventTime, checkTime);
            }
        }

        @Override
        public void onTimer(long timestamp, OnTimerContext ctx, Collector<DeviceAlert> out) throws Exception {
            Long lastEventTime = lastEventTimeState.value();

            if (lastEventTime != null) {
                long timeSinceLastEvent = timestamp - lastEventTime;

                if (timeSinceLastEvent >= INACTIVITY_THRESHOLD_MS) {
                    // Device is inactive, generate alert
                    String deviceId = ctx.getCurrentKey();
                    DeviceAlert alert = new DeviceAlert(deviceId, lastEventTime);
                    out.collect(alert);
                    LOG.info("Generated alert for inactive device: {}, last event at {}",
                            deviceId, lastEventTime);

                    // Register next check
                    long nextCheckTime = timestamp + WINDOW_SIZE_MS;
                    ctx.timerService().registerEventTimeTimer(nextCheckTime);
                    nextCheckTimeState.update(nextCheckTime);
                } else {
                    // Event came in after timer was set, reschedule
                    long nextCheckTime = lastEventTime + INACTIVITY_THRESHOLD_MS;
                    ctx.timerService().registerEventTimeTimer(nextCheckTime);
                    nextCheckTimeState.update(nextCheckTime);
                }
            }
        }
    }

    /**
     * Aggregates alerts within a window (for batching alerts every 30 seconds).
     */
    public static class AlertAggregator extends ProcessAllWindowFunction<DeviceAlert, DeviceAlert, TimeWindow> {
        @Override
        public void process(Context context, Iterable<DeviceAlert> alerts, Collector<DeviceAlert> out) {
            for (DeviceAlert alert : alerts) {
                out.collect(alert);
            }
        }
    }

    private static String getParameterOrDefault(String[] args, String key, String defaultValue) {
        for (String arg : args) {
            if (arg.startsWith("--" + key + "=")) {
                return arg.substring(("--" + key + "=").length());
            }
        }
        return defaultValue;
    }
}
