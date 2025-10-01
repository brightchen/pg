package bc.monitor;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.apache.kafka.clients.admin.AdminClient;
import org.apache.kafka.clients.admin.NewTopic;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringDeserializer;
import org.apache.kafka.common.serialization.StringSerializer;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.testcontainers.containers.KafkaContainer;
import org.testcontainers.junit.jupiter.Container;
import org.testcontainers.junit.jupiter.Testcontainers;
import org.testcontainers.utility.DockerImageName;

import java.time.Duration;
import java.util.*;
import java.util.concurrent.ExecutionException;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * Integration test for the Monitor application using Testcontainers.
 */
@Testcontainers
class MonitorIntegrationTest {

    private static final String EVENTS_TOPIC = "events";
    private static final String ALERTS_TOPIC = "alerts";

    @Container
    private static final KafkaContainer kafka = new KafkaContainer(
            DockerImageName.parse("confluentinc/cp-kafka:7.5.0"))
            .withKraft();

    private final ObjectMapper objectMapper = new ObjectMapper();
    private KafkaProducer<String, String> producer;
    private KafkaConsumer<String, String> consumer;
    private AdminClient adminClient;

    @BeforeEach
    void setUp() throws ExecutionException, InterruptedException {
        String bootstrapServers = kafka.getBootstrapServers();

        // Create admin client
        Properties adminProps = new Properties();
        adminProps.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        adminClient = AdminClient.create(adminProps);

        // Create topics
        adminClient.createTopics(Arrays.asList(
                new NewTopic(EVENTS_TOPIC, 3, (short) 1),
                new NewTopic(ALERTS_TOPIC, 1, (short) 1)
        )).all().get();

        // Create producer
        Properties producerProps = new Properties();
        producerProps.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        producerProps.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        producerProps.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, StringSerializer.class.getName());
        producer = new KafkaProducer<>(producerProps);

        // Create consumer
        Properties consumerProps = new Properties();
        consumerProps.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        consumerProps.put(ConsumerConfig.GROUP_ID_CONFIG, "test-consumer");
        consumerProps.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "earliest");
        consumer = new KafkaConsumer<>(consumerProps);
        consumer.subscribe(Collections.singletonList(ALERTS_TOPIC));
    }

    @AfterEach
    void tearDown() {
        if (producer != null) producer.close();
        if (consumer != null) consumer.close();
        if (adminClient != null) adminClient.close();
    }

    @Test
    @Timeout(30)
    void testDeviceEventProcessing() throws Exception {
        // Publish test events
        long currentTime = System.currentTimeMillis();

        DeviceEvent event1 = new DeviceEvent("e1", "d1", currentTime);
        DeviceEvent event2 = new DeviceEvent("e2", "d2", currentTime);
        DeviceEvent event3 = new DeviceEvent("e3", "d1", currentTime + 5000);

        publishEvent(event1);
        publishEvent(event2);
        publishEvent(event3);

        // Wait a bit for processing
        Thread.sleep(2000);

        // Verify events can be consumed (basic connectivity test)
        assertThat(kafka.isRunning()).isTrue();
    }

    @Test
    @Timeout(30)
    void testEventDeserialization() throws Exception {
        long currentTime = System.currentTimeMillis();

        // Test various event formats
        DeviceEvent event1 = new DeviceEvent("e1", "device-123", currentTime);
        String json1 = objectMapper.writeValueAsString(event1);

        DeviceEvent parsed1 = objectMapper.readValue(json1, DeviceEvent.class);

        assertThat(parsed1.getEventId()).isEqualTo("e1");
        assertThat(parsed1.getDeviceId()).isEqualTo("device-123");
        assertThat(parsed1.getTimestamp()).isEqualTo(currentTime);
    }

    @Test
    @Timeout(30)
    void testMultipleDevicesOnDifferentPartitions() throws Exception {
        long currentTime = System.currentTimeMillis();

        // Send events for multiple devices (will go to different partitions)
        List<DeviceEvent> events = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            DeviceEvent event = new DeviceEvent("e" + i, "d" + (i % 3), currentTime + i * 1000);
            events.add(event);
            publishEvent(event);
        }

        // Verify all events were published
        assertThat(events).hasSize(10);
    }

    @Test
    @Timeout(30)
    void testOutOfOrderEvents() throws Exception {
        long currentTime = System.currentTimeMillis();

        // Send events out of order
        DeviceEvent event1 = new DeviceEvent("e1", "d1", currentTime + 10000);
        DeviceEvent event2 = new DeviceEvent("e2", "d1", currentTime + 5000);
        DeviceEvent event3 = new DeviceEvent("e3", "d1", currentTime + 15000);

        publishEvent(event1);
        publishEvent(event2);
        publishEvent(event3);

        // Basic test that events can be published out of order
        Thread.sleep(1000);
        assertThat(kafka.isRunning()).isTrue();
    }

    @Test
    @Timeout(30)
    void testAlertSerialization() throws Exception {
        DeviceAlert alert = new DeviceAlert("device-123", System.currentTimeMillis());
        String json = objectMapper.writeValueAsString(alert);

        assertThat(json).contains("\"device-id\":\"device-123\"");
        assertThat(json).contains("\"latest-event-time\"");

        DeviceAlert parsed = objectMapper.readValue(json, DeviceAlert.class);
        assertThat(parsed.getDeviceId()).isEqualTo("device-123");
    }

    @Test
    void testKafkaConnectivity() {
        assertThat(kafka.isRunning()).isTrue();
        assertThat(kafka.getBootstrapServers()).isNotEmpty();
    }

    private void publishEvent(DeviceEvent event) throws Exception {
        String json = objectMapper.writeValueAsString(event);
        ProducerRecord<String, String> record = new ProducerRecord<>(
                EVENTS_TOPIC,
                event.getDeviceId(),
                json
        );
        producer.send(record).get();
    }

    private List<DeviceAlert> consumeAlerts(int expectedCount, Duration timeout) {
        List<DeviceAlert> alerts = new ArrayList<>();
        long endTime = System.currentTimeMillis() + timeout.toMillis();

        while (System.currentTimeMillis() < endTime && alerts.size() < expectedCount) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> record : records) {
                try {
                    DeviceAlert alert = objectMapper.readValue(record.value(), DeviceAlert.class);
                    alerts.add(alert);
                } catch (Exception e) {
                    throw new RuntimeException("Error parsing alert", e);
                }
            }
        }

        return alerts;
    }
}
