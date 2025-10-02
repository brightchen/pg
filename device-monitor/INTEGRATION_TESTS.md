# Integration Tests - Smart Kafka Management

## Overview

The integration tests now use a smart Kafka management approach that provides:

1. **Persistent Kafka cluster** - Reuses running Kafka instead of starting/stopping for each test
2. **Auto-start capability** - Automatically starts Kafka via docker-compose if not running
3. **Topic management** - Creates test topics if missing, cleans them before tests
4. **No teardown** - Keeps Kafka running after tests for faster subsequent runs
5. **Configurable** - Can point to any Kafka cluster (local, K8s, etc.)

## Architecture

### KafkaTestHelper

New helper class: `monitor/src/integration-test/java/bc/monitor/KafkaTestHelper.java`

**Responsibilities:**
- Check if Kafka is accessible at configured bootstrap servers
- Start Kafka with docker-compose if not running
- Manage test topics (create if missing)
- Clean up messages in topics before tests (using Kafka Admin API)

**Key Methods:**
- `ensureKafkaRunning()` - Ensure Kafka cluster is available
- `ensureTopicsExistAndClean(String... topics)` - Manage topics and clean messages
- `cleanupTopic(String topic)` - Delete all records in a topic
- `isKafkaRunning()` - Check Kafka accessibility
- `startKafkaWithDockerCompose()` - Launch Kafka via docker-compose

### MonitorIntegrationTest

Updated test class: `monitor/src/integration-test/java/bc/monitor/MonitorIntegrationTest.java`

**Changes:**
- Removed Testcontainers (no longer creates ephemeral Kafka instances)
- Uses `KafkaTestHelper` to manage persistent Kafka cluster
- Uses separate test topics: `events-test`, `alerts-test`
- Cleans topics before each test method
- Added test for topic cleanup functionality

## How It Works

### Test Lifecycle

1. **@BeforeAll** (once per test class):
   ```java
   - Create KafkaTestHelper
   - Check if Kafka is running at localhost:9094
   - If not, start with: docker compose up -d kafka kafka-setup
   - Create test topics if they don't exist
   - Clean up any existing messages
   ```

2. **@BeforeEach** (before each test):
   ```java
   - Clean up test topics (delete all records)
   - Create new producer and consumer instances
   ```

3. **@AfterEach** (after each test):
   ```java
   - Close producer and consumer
   ```

4. **@AfterAll** (once per test class):
   ```java
   - Close KafkaTestHelper (admin client)
   - Log message if Kafka was started (remains running)
   ```

### Topic Cleanup Strategy

Instead of deleting and recreating topics, we delete all records:

```java
// Get end offsets for all partitions
Map<TopicPartition, Long> endOffsets = consumer.endOffsets(partitions);

// Delete records up to end offset
Map<TopicPartition, RecordsToDelete> recordsToDelete = new HashMap<>();
for (Map.Entry<TopicPartition, Long> entry : endOffsets.entrySet()) {
    recordsToDelete.put(entry.getKey(), RecordsToDelete.beforeOffset(entry.getValue()));
}

adminClient.deleteRecords(recordsToDelete).all().get();
```

**Benefits:**
- Preserves topic configuration (partitions, replication)
- Faster than delete + recreate
- Resets consumer offsets by using unique consumer groups

## Configuration

### Default Configuration

```properties
kafka.bootstrap.servers=localhost:9094
```

### Custom Configuration

**Via System Property:**
```bash
mvn verify -Dkafka.bootstrap.servers=your-kafka:9092
```

**Via Environment Variable:**
```bash
export KAFKA_BOOTSTRAP_SERVERS=your-kafka:9092
make integration-test
```

**Via Script Argument:**
```bash
./scripts/integration-test.sh your-kafka:9092
```

## Usage Examples

### Run with auto-start

```bash
# If Kafka not running, it will be started automatically
make integration-test
```

### Run with existing Kafka

```bash
# Start Kafka first
docker compose up -d kafka kafka-setup

# Run tests (will detect running Kafka)
make integration-test
```

### Run with K8s Kafka

```bash
# Port-forward to K8s Kafka
kubectl port-forward -n device-monitor-dev svc/kafka 9094:9092

# Run tests pointing to K8s
export KAFKA_BOOTSTRAP_SERVERS=localhost:9094
make integration-test
```

### Clean slate

```bash
# Stop and remove Kafka completely
docker compose down -v

# Next test run will start fresh Kafka
make integration-test
```

## Test Topics

The integration tests use separate topics from production:

| Topic | Partitions | Purpose |
|-------|-----------|---------|
| `events-test` | 3 | Test device events |
| `alerts-test` | 1 | Test alert output |

These topics are:
- Created automatically if missing
- Cleaned before each test
- Kept after tests complete
- Separate from production `events` and `alerts` topics

## Performance Benefits

### Before (Testcontainers)

```
Test Run 1: Start Kafka (30s) + Run Tests (10s) = 40s
Test Run 2: Start Kafka (30s) + Run Tests (10s) = 40s
Test Run 3: Start Kafka (30s) + Run Tests (10s) = 40s
```

### After (Persistent Kafka)

```
Test Run 1: Start Kafka (30s) + Run Tests (10s) = 40s
Test Run 2: Run Tests (10s) = 10s (75% faster)
Test Run 3: Run Tests (10s) = 10s (75% faster)
```

## Troubleshooting

### Kafka won't start

```bash
# Check Docker is running
docker ps

# Check port is available
lsof -i :9094

# Check docker-compose.yaml exists
ls -la ../docker-compose.yaml
```

### Tests fail with connection errors

```bash
# Verify Kafka is accessible
docker ps | grep kafka

# Check Kafka logs
docker logs kafka

# Test connection manually
docker exec -it kafka kafka-topics.sh --bootstrap-server localhost:9092 --list
```

### Topics not being created

```bash
# Check topics exist
docker exec -it kafka kafka-topics.sh --bootstrap-server localhost:9092 --list

# Manually create topics
docker exec -it kafka kafka-topics.sh \
  --bootstrap-server localhost:9092 \
  --create --topic events-test --partitions 3 --replication-factor 1
```

### Clean up everything

```bash
# Stop and remove Kafka with all data
docker compose down -v

# Remove all containers and networks
docker system prune -f
```

## Future Enhancements

Possible improvements:

1. **Parallel test execution** - Run tests in parallel with topic isolation
2. **Schema validation** - Validate event/alert schemas before tests
3. **Performance tests** - Add tests for throughput and latency
4. **Chaos tests** - Test behavior when Kafka fails during processing
5. **Multi-cluster tests** - Test with multiple Kafka clusters

## Related Files

| File | Purpose |
|------|---------|
| `monitor/src/integration-test/java/bc/monitor/KafkaTestHelper.java` | Kafka cluster management |
| `monitor/src/integration-test/java/bc/monitor/MonitorIntegrationTest.java` | Integration tests |
| `scripts/integration-test.sh` | Test runner script |
| `docker-compose.yaml` | Kafka cluster definition |
| `README.md` | User documentation |
| `claude/CLAUDE.md` | Developer documentation |
