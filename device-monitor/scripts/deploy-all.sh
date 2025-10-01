#!/bin/bash
set -e

# Deploy entire system (Kafka + Flink monitor) to Kubernetes

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
echo "Deploying entire system to $ENV environment"
echo "========================================="

# Ensure namespace exists
echo "Creating namespace $NAMESPACE..."
kubectl create namespace "$NAMESPACE" --dry-run=client -o yaml | kubectl apply -f -

# Deploy Kafka
echo ""
echo "========================================="
echo "Deploying Kafka cluster..."
echo "========================================="
cd "$PROJECT_ROOT/infrastructure/kafka"

# Add Bitnami repo if not already added
helm repo add bitnami https://charts.bitnami.com/bitnami 2>/dev/null || true
helm repo update

helm upgrade --install kafka-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait \
    --timeout 10m

echo "Kafka deployed successfully!"

# Wait for Kafka to be fully ready
echo "Waiting for Kafka to be ready..."
sleep 30

# Build and package monitor if not already done
if [ ! -f "$PROJECT_ROOT/monitor/target/monitor-1.0.0.jar" ]; then
    echo ""
    echo "========================================="
    echo "Building monitor application..."
    echo "========================================="
    "$SCRIPT_DIR/package.sh"
fi

# Deploy Flink monitor
echo ""
echo "========================================="
echo "Deploying Flink monitor application..."
echo "========================================="
cd "$PROJECT_ROOT/infrastructure/flink"

helm upgrade --install monitor-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait \
    --timeout 10m

echo ""
echo "========================================="
echo "Deployment completed successfully!"
echo "========================================="
echo ""
echo "Environment: $ENV"
echo "Namespace: $NAMESPACE"
echo ""
echo "Useful commands:"
echo "  List pods: kubectl get pods -n $NAMESPACE"
echo "  Kafka logs: kubectl logs -n $NAMESPACE -l app.kubernetes.io/name=kafka"
echo "  Flink JobManager logs: kubectl logs -n $NAMESPACE -l app=flink,component=jobmanager"
echo "  Flink TaskManager logs: kubectl logs -n $NAMESPACE -l app=flink,component=taskmanager"
echo "  Access Flink UI: kubectl port-forward -n $NAMESPACE svc/monitor-$ENV-jobmanager 8081:8081"
echo ""
