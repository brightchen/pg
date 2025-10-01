#!/bin/bash
set -e

# Integration test script for monitor application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

KAFKA_BOOTSTRAP_SERVERS=${1:-"localhost:9094"}

echo "========================================="
echo "Running Integration Tests"
echo "========================================="
echo ""
echo "Configuration:"
echo "  Kafka Bootstrap Servers: $KAFKA_BOOTSTRAP_SERVERS"
echo "  Docker Required: Yes"
echo ""
echo "The tests will:"
echo "  1. Check if Kafka is running at $KAFKA_BOOTSTRAP_SERVERS"
echo "  2. Start Kafka with docker-compose if not running"
echo "  3. Create/verify test topics exist"
echo "  4. Clean up test topics before running tests"
echo "  5. Run all integration tests"
echo "  6. Keep Kafka running after tests complete"
echo ""
echo "To use a different Kafka cluster:"
echo "  export KAFKA_BOOTSTRAP_SERVERS=your-kafka:9092"
echo "  or pass as argument: $0 your-kafka:9092"
echo ""

cd "$PROJECT_ROOT/monitor"

# Run integration tests with custom Kafka bootstrap servers
mvn verify -DskipUnitTests=true \
    -Dkafka.bootstrap.servers="$KAFKA_BOOTSTRAP_SERVERS"

echo ""
echo "========================================="
echo "All integration tests passed!"
echo "========================================="
echo ""
echo "Note: If Kafka was started by tests, it is still running."
echo "To stop it, run: cd $PROJECT_ROOT && docker compose down"
echo ""
