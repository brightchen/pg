#!/bin/bash
set -e

# Package script for monitor application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Packaging monitor application..."
cd "$PROJECT_ROOT/monitor"
mvn clean package -DskipTests

echo "Packaging event generator..."
cd "$PROJECT_ROOT/event-generator"
mvn clean package -DskipTests

echo "Package completed successfully!"
echo "Monitor JAR: $PROJECT_ROOT/monitor/target/monitor-1.0.0.jar"
echo "Event Generator JAR: $PROJECT_ROOT/event-generator/target/event-generator-1.0.0.jar"
