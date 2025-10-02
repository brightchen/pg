#!/bin/bash
set -e

# Deploy complete IoT monitoring system (Kafka + Flink + Monitoring Stack)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

ENV=${1:-dev}
NAMESPACE="device-monitor-$ENV"

if [[ ! "$ENV" =~ ^(dev|qa|prod)$ ]]; then
    echo "Usage: $0 [dev|qa|prod]"
    echo "Default: dev"
    exit 1
fi

echo "========================================="
echo "Deploying Complete IoT Monitoring System"
echo "Environment: $ENV"
echo "========================================="

# Deploy Kafka + Flink monitor
echo ""
echo "Step 1/2: Deploying Kafka and Flink Monitor..."
"$SCRIPT_DIR/deploy-all.sh" "$ENV"

# Deploy Monitoring Stack
echo ""
echo "Step 2/2: Deploying Monitoring Stack..."
"$SCRIPT_DIR/deploy-monitoring.sh" "$ENV"

echo ""
echo "========================================="
echo "Complete System Deployment Finished!"
echo "========================================="
echo ""
echo "System Architecture:"
echo ""
echo "  IoT Devices --> Kafka (events) --> Flink Monitor --> Kafka (alerts)"
echo "                                                             |"
echo "                                                             v"
echo "                                                          Vector"
echo "                                                             |"
echo "                                                             v"
echo "                                                       TimescaleDB <-- Grafana"
echo ""
echo "Deployed Components:"
echo "  - Kafka Cluster"
echo "  - Flink Monitor Application"
echo "  - Vector (Data Pipeline)"
echo "  - TimescaleDB (Time-Series Database)"
echo "  - Grafana (Visualization)"
echo ""
echo "Quick Start:"
echo ""
echo "1. Access Grafana Dashboard:"
echo "   kubectl port-forward -n $NAMESPACE svc/grafana-$ENV 3000:80"
echo "   Open: http://localhost:3000"
echo "   Username: admin"
echo "   Password: (check infrastructure/grafana/values-$ENV.yaml)"
echo ""
echo "2. View Flink UI:"
echo "   kubectl port-forward -n $NAMESPACE svc/monitor-$ENV-jobmanager 8081:8081"
echo "   Open: http://localhost:8081"
echo ""
echo "3. Check all pods:"
echo "   kubectl get pods -n $NAMESPACE"
echo ""
echo "4. Generate test events:"
echo "   # Port-forward to Kafka"
echo "   kubectl port-forward -n $NAMESPACE svc/kafka-$ENV 9094:9092"
echo "   # Run event generator"
echo "   make run-generator"
echo ""
