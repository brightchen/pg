# IoT Device Monitoring System

An Apache Flink-based system for monitoring IoT device events and generating alerts when devices become inactive.

## System Overview

This system monitors events from IoT devices sent via Kafka and generates alerts when a device hasn't sent an event for more than 1 minute. Alerts are generated every 30 seconds as a list of inactive devices.

### Components

1. **Monitor Application** (`monitor/`) - Apache Flink job that processes device events
2. **Event Generator** (`event-generator/`) - Test tool to generate mock device events
3. **Kafka Cluster** (`infrastructure/kafka/`) - Message broker for events and alerts
4. **Flink Deployment** (`infrastructure/flink/`) - Kubernetes deployment for Flink

### Event Flow

1. Devices send events to Kafka topic `events` with format: `{"event-id": "e123", "device-id": "d456", "time": 1234567890}`
2. Flink Monitor application reads events, tracks last event time per device
3. Alerts generated for devices inactive >1 minute
4. Alerts written to Kafka topic `alerts` with format: `{"device-id": "d456", "latest-event-time": 1234567890}`

## Prerequisites

- **Java 21** - Required for building and running applications
- **Maven 3.8+** - Build tool
- **Docker** - For local testing and integration tests
- **Kubernetes cluster** - For deployment (dev/qa/prod)
- **Helm 3** - Kubernetes package manager
- **kubectl** - Kubernetes CLI

## Quick Start

### 1. Build the Project

```bash
# Build all applications
make build

# Or use script directly
./scripts/build.sh
```

### 2. Run Tests

```bash
# Unit tests
make test

# Integration tests (requires Docker)
make integration-test
```

### 3. Package Applications

```bash
# Create JAR files
make package
```

### 4. Local Development with Docker Compose

```bash
# Start Kafka and Flink locally
make docker-up

# Access Flink UI at http://localhost:8081

# Run event generator to produce test events
make run-generator

# Stop everything
make docker-down
```

## Development Commands

### Build & Test

```bash
make build              # Build all applications
make test               # Run unit tests
make integration-test   # Run integration tests (requires Docker)
make package            # Package applications as JAR files
make clean              # Clean build artifacts
```

### Local Development

```bash
make docker-up          # Start local Kafka + Flink
make docker-down        # Stop local environment
make run-generator      # Run event generator
```

### Deploy to Kubernetes

#### Deploy Monitor Application Only

```bash
make deploy-dev         # Deploy to dev environment
make deploy-qa          # Deploy to qa environment
make deploy-prod        # Deploy to prod environment
```

#### Deploy Full System (Kafka + Monitor)

```bash
make deploy-all-dev     # Deploy everything to dev
make deploy-all-qa      # Deploy everything to qa
make deploy-all-prod    # Deploy everything to prod
```

## Project Structure

```
.
├── monitor/                          # Flink monitor application
│   ├── src/
│   │   ├── main/java/bc/monitor/
│   │   │   ├── Event.java           # Event interface
│   │   │   ├── DeviceEvent.java     # Device event implementation
│   │   │   ├── DeviceAlert.java     # Alert model
│   │   │   └── Monitor.java         # Main Flink job
│   │   ├── test/java/               # Unit tests
│   │   └── integration-test/java/   # Integration tests
│   └── pom.xml
│
├── event-generator/                  # Event generator tool
│   ├── src/main/java/bc/eventgen/
│   │   ├── DeviceEvent.java
│   │   └── EventGenerator.java      # Main generator class
│   └── pom.xml
│
├── infrastructure/
│   ├── kafka/                        # Kafka Helm chart
│   │   ├── Chart.yaml
│   │   ├── values-dev.yaml
│   │   ├── values-qa.yaml
│   │   └── values-prod.yaml
│   │
│   └── flink/                        # Flink Helm chart
│       ├── Chart.yaml
│       ├── templates/
│       │   ├── configmap.yaml
│       │   ├── jobmanager-deployment.yaml
│       │   ├── jobmanager-service.yaml
│       │   ├── taskmanager-deployment.yaml
│       │   └── serviceaccount.yaml
│       ├── values-dev.yaml
│       ├── values-qa.yaml
│       └── values-prod.yaml
│
├── scripts/
│   ├── build.sh                      # Build applications
│   ├── test.sh                       # Run unit tests
│   ├── integration-test.sh           # Run integration tests
│   ├── package.sh                    # Package JARs
│   ├── deploy-monitor.sh             # Deploy monitor to K8s
│   ├── deploy-all.sh                 # Deploy full system to K8s
│   └── run-event-generator.sh        # Run event generator
│
├── docker-compose.yaml               # Local development environment
├── Makefile                          # Easy command access
└── README.md                         # This file
```

## Architecture Details

### Flink Processing

1. **Event Time Processing**: Uses watermarks to handle out-of-order events
2. **Keyed State**: Maintains per-device state for last event timestamp
3. **Checkpointing**: Exactly-once semantics with 10-second checkpoints
4. **Windowing**: 30-second tumbling windows for alert aggregation

### Kafka Configuration

- **Events Topic**: 3 partitions, handles device events
- **Alerts Topic**: 1 partition, outputs alerts
- **KRaft Mode**: No Zookeeper dependency

### Resource Requirements

#### Development
- Kafka: 1 replica, 1-2Gi memory
- Flink JobManager: 2Gi memory
- Flink TaskManager: 2 replicas, 2Gi each

#### Production
- Kafka: 3 replicas, 4-8Gi memory each
- Flink JobManager: 4Gi memory
- Flink TaskManager: 3 replicas, 4Gi each

## Configuration

### Monitor Application Parameters

The Monitor application accepts these command-line arguments:

- `--kafka.bootstrap.servers` - Kafka bootstrap servers (default: localhost:9092)
- `--events.topic` - Input topic name (default: events)
- `--alerts.topic` - Output topic name (default: alerts)
- `--checkpoint.dir` - Checkpoint directory (default: file:///tmp/flink-checkpoints)
- `--parallelism` - Parallelism level (default: 2)

### Event Generator Parameters

- `--kafka.bootstrap.servers` - Kafka bootstrap servers
- `--topic` - Topic to publish to (default: events)
- `--num.devices` - Number of devices to simulate (default: 10)
- `--event.interval.ms` - Interval between events (default: 10)
- `--duration.seconds` - How long to run (default: 60)
- `--failure.probability` - Probability of device failure (default: 0.1)

## Monitoring & Operations

### Access Flink UI

```bash
# Local Docker
http://localhost:8081

# Kubernetes
kubectl port-forward -n device-monitor-dev svc/monitor-dev-jobmanager 8081:8081
# Then access http://localhost:8081
```

### View Logs

```bash
# Kafka logs
kubectl logs -n device-monitor-dev -l app.kubernetes.io/name=kafka

# Flink JobManager logs
kubectl logs -n device-monitor-dev -l app=flink,component=jobmanager

# Flink TaskManager logs
kubectl logs -n device-monitor-dev -l app=flink,component=taskmanager
```

### Check Kafka Topics

```bash
# List topics
kubectl exec -n device-monitor-dev kafka-0 -- kafka-topics.sh \
  --bootstrap-server localhost:9092 --list

# Describe topic
kubectl exec -n device-monitor-dev kafka-0 -- kafka-topics.sh \
  --bootstrap-server localhost:9092 --describe --topic events
```

## Troubleshooting

### Build Issues

- Ensure Java 21 is installed: `java -version`
- Ensure Maven is installed: `mvn -version`
- Clean and rebuild: `make clean && make build`

### Docker Compose Issues

- Ensure Docker is running
- Check logs: `docker compose logs kafka` or `docker compose logs jobmanager`
- Restart: `make docker-down && make docker-up`

### Kubernetes Deployment Issues

- Check pod status: `kubectl get pods -n device-monitor-dev`
- View pod logs: `kubectl logs -n device-monitor-dev <pod-name>`
- Describe pod: `kubectl describe pod -n device-monitor-dev <pod-name>`

## License

Internal project for IoT device monitoring.
