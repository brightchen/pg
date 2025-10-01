# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an IoT Device Monitoring system that uses Apache Flink to process events from IoT devices via Kafka. The system generates alerts when devices fail to send events within expected timeframes.

**Key Requirements:**
- IoT devices send events every 10ms to Kafka topic "events" with format: `{"event-id": "e3445", "device-id": "d123", "time": epoch-time}`
- Flink application monitors these events and alerts if a device hasn't sent an event for more than 1 minute
- Alerts are generated every 30 seconds as a list of inactive devices
- Alerts are written to Kafka topic "alerts" with format: `{"device-id": "...", "latest-event-time": "..."}`
- System must handle: events from same device going to different Kafka partitions, out-of-order events, and delayed events
- Generate this Flink application with Java 21
- The name of this application is monitor

## Architecture

The system consists of three main components:

1. **Kafka Cluster**
   - Uses Bitnami Kafka Helm chart with KRaft mode (no Zookeeper)
   - 1 replicas with 3 partitions for event distribution
   - Topics: "events" (input) and "alerts" (output)

2. **Flink Application** (`monitor/`)
   - Java-based Flink job: `bc.monitor.Monitor`
   - Deployed with separate JobManager and TaskManager pods
   - Configured for exactly-once checkpointing every 10 seconds
   - Handles stateful processing to track last event time per device
   - abstract the Event interface. The Event implement should have information timestamp as epoc time; event id as string; event group as string.
   - For this specific implementation of Event, device id is event group
   - generate unit test cases
   - generate integration test cases. Each integration test case can run inside a docker. unit test cases will create events, publish to kafka, The monitor application read data from kafka and output, and then integration test cases verify the result

3. **Event Generator** (`event-generator/`)
   - Java-based tool to generate mock IoT device events for testing
   - Configurable number of devices, event intervals, and failure scenarios
   - Main class: `bc.eventgen.EventGenerator`

## Key Flink Configuration
- **Checkpointing**: Enabled with 10-second interval, exactly-once mode
- **State Backend**: Filesystem-based
- **Parallelism**: 2 (configurable)
- **Task Slots**: 4 per TaskManager
- **Resource Requirements**:
  - JobManager: 1-2 CPU, 2-4Gi memory
  - TaskManager: 1-2 CPU, 2-4Gi memory (2 replicas)

## Important Considerations for Flink Development

1. **Keyed State**: Use device-id as key to maintain per-device event timestamps across partitions
2. **Event Time Processing**: Handle out-of-order and delayed events using watermarks
3. **Window Processing**: Use 30-second tumbling/sliding windows for alert generation
4. **State Management**: Store last-seen timestamp for each device to detect inactivity (>1 minute threshold)
5. **Kafka Consumer**: Must handle multiple partitions and partition reassignment gracefully


## Deployment
- All deployment managed by helm
- run in k8s
- provides dev, qa and prod enviroment

## Project Structure

```
.
├── monitor/                    # Flink monitor application (Java 21, Maven)
│   ├── src/main/java/bc/monitor/
│   │   ├── Event.java         # Event interface
│   │   ├── DeviceEvent.java   # Device event implementation
│   │   ├── DeviceAlert.java   # Alert model
│   │   └── Monitor.java       # Main Flink job (entry point)
│   ├── src/test/java/         # Unit tests
│   └── src/integration-test/java/  # Integration tests (use Testcontainers)
│
├── event-generator/            # Event generator tool (Java 21, Maven)
│   └── src/main/java/bc/eventgen/
│       └── EventGenerator.java
│
├── infrastructure/
│   ├── kafka/                  # Kafka Helm chart (Bitnami dependency)
│   │   ├── values-dev.yaml
│   │   ├── values-qa.yaml
│   │   └── values-prod.yaml
│   └── flink/                  # Flink Helm chart
│       ├── templates/          # K8s manifests
│       ├── values-dev.yaml
│       ├── values-qa.yaml
│       └── values-prod.yaml
│
└── scripts/                    # Build and deployment scripts
```

## Common Commands

All commands should be run from the project root directory.

### Build & Test

```bash
# Build all applications
make build
# Or: ./scripts/build.sh

# Run unit tests
make test
# Or: ./scripts/test.sh

# Run integration tests (requires Docker)
make integration-test
# Or: ./scripts/integration-test.sh

# Package applications as JARs
make package
# Or: ./scripts/package.sh

# Clean build artifacts
make clean
```

### Local Development

```bash
# Start Kafka + Flink locally with Docker Compose
make docker-up

# Run event generator to produce test events
make run-generator
# Or: ./scripts/run-event-generator.sh [kafka-servers] [num-devices] [interval-ms] [duration-sec] [failure-prob]

# Stop local environment
make docker-down

# Access Flink UI: http://localhost:8081
# Kafka bootstrap servers: localhost:9094
```

### Deploy to Kubernetes

```bash
# Deploy monitor application only
make deploy-dev        # Deploy to dev
make deploy-qa         # Deploy to qa
make deploy-prod       # Deploy to prod

# Deploy full system (Kafka + Monitor)
make deploy-all-dev    # Deploy everything to dev
make deploy-all-qa     # Deploy everything to qa
make deploy-all-prod   # Deploy everything to prod
```

### Individual Scripts

```bash
./scripts/build.sh                    # Build applications
./scripts/test.sh                     # Unit tests
./scripts/integration-test.sh         # Integration tests
./scripts/package.sh                  # Package JARs
./scripts/deploy-monitor.sh [env]     # Deploy monitor to k8s
./scripts/deploy-all.sh [env]         # Deploy full system to k8s
./scripts/run-event-generator.sh      # Run event generator
```

## Key Implementation Details

### Monitor Application (monitor/src/main/java/bc/monitor/Monitor.java)

- **DeviceStateTracker**: KeyedProcessFunction that tracks last event time per device and registers timers for inactivity checks
- **AlertAggregator**: ProcessWindowFunction that batches alerts within 30-second windows
- Uses event time processing with 10-second watermark delay for out-of-order events
- Keyed by device-id to maintain state across Kafka partitions
- Command-line arguments: `--kafka.bootstrap.servers`, `--events.topic`, `--alerts.topic`, `--checkpoint.dir`, `--parallelism`

### Event Generator (event-generator/src/main/java/bc/eventgen/EventGenerator.java)

- Simulates multiple IoT devices sending events at configurable intervals
- Can simulate device failures (stops sending events) and recovery
- Command-line arguments: `--kafka.bootstrap.servers`, `--topic`, `--num.devices`, `--event.interval.ms`, `--duration.seconds`, `--failure.probability`

## Integration Tests

The integration tests (monitor/src/integration-test/java/) use a smart Kafka management approach via `KafkaTestHelper`:

1. Check if Kafka is running at configured bootstrap servers (default: `localhost:9094`)
2. If not running, automatically start Kafka using docker-compose
3. Verify test topics exist (`events-test`, `alerts-test`), create if missing
4. Clean up messages in test topics before each test (using Kafka Admin API to delete records)
5. Keep Kafka running after tests complete (no teardown)

**Configuration:**
- Set via system property: `-Dkafka.bootstrap.servers=your-kafka:9092`
- Or environment variable: `KAFKA_BOOTSTRAP_SERVERS=your-kafka:9092`
- Default: `localhost:9094`

**Running tests:**
```bash
make integration-test
# Or with custom Kafka:
./scripts/integration-test.sh your-kafka:9092
```

This approach provides faster test execution on subsequent runs since Kafka doesn't need to restart.