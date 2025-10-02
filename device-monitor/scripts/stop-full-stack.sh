#!/bin/bash
set -e

# Stop complete IoT monitoring system

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
echo "Stopping Complete IoT Monitoring System"
echo "Environment: $ENV"
echo "========================================="
echo ""
echo "WARNING: This will remove ALL components:"
echo "  - Monitoring Stack (Grafana, Vector, TimescaleDB)"
echo "  - Flink Monitor Application"
echo "  - Kafka Cluster"
echo ""
echo "Data in PersistentVolumes will be preserved unless deleted manually."
echo ""
read -p "Are you sure you want to continue? (yes/no): " confirm

if [[ "$confirm" != "yes" ]]; then
    echo "Cancelled."
    exit 0
fi

# Stop Monitoring Stack
echo ""
echo "Step 1/2: Stopping Monitoring Stack..."
"$SCRIPT_DIR/stop-monitoring.sh" "$ENV" <<< "yes"

# Stop Kafka and Flink
echo ""
echo "Step 2/2: Stopping Kafka and Flink..."

echo "Uninstalling Flink Monitor..."
helm uninstall monitor-$ENV --namespace "$NAMESPACE" 2>/dev/null || echo "Flink Monitor not found"

echo "Uninstalling Kafka..."
helm uninstall kafka-$ENV --namespace "$NAMESPACE" 2>/dev/null || echo "Kafka not found"

echo ""
echo "========================================="
echo "Complete System Stopped"
echo "========================================="
echo ""
echo "To view remaining resources:"
echo "kubectl get all,pvc -n $NAMESPACE"
echo ""
echo "To completely delete the namespace and all resources (IRREVERSIBLE):"
echo "kubectl delete namespace $NAMESPACE"
echo ""
