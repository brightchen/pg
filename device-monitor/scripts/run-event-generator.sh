#!/bin/bash
set -e

# Run event generator locally

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

KAFKA_SERVERS=${1:-localhost:9094}
NUM_DEVICES=${2:-10}
EVENT_INTERVAL=${3:-10}
DURATION=${4:-60}
FAILURE_PROB=${5:-0.1}

echo "Running event generator..."
echo "  Kafka: $KAFKA_SERVERS"
echo "  Devices: $NUM_DEVICES"
echo "  Event interval: ${EVENT_INTERVAL}ms"
echo "  Duration: ${DURATION}s"
echo "  Failure probability: $FAILURE_PROB"

# Build if needed
if [ ! -f "$PROJECT_ROOT/event-generator/target/event-generator-1.0.0.jar" ]; then
    echo "Building event generator..."
    cd "$PROJECT_ROOT/event-generator"
    mvn clean package -DskipTests
fi

# Run event generator
java -jar "$PROJECT_ROOT/event-generator/target/event-generator-1.0.0.jar" \
    --kafka.bootstrap.servers="$KAFKA_SERVERS" \
    --topic=events \
    --num.devices="$NUM_DEVICES" \
    --event.interval.ms="$EVENT_INTERVAL" \
    --duration.seconds="$DURATION" \
    --failure.probability="$FAILURE_PROB"
