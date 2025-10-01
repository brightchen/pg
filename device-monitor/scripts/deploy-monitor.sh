#!/bin/bash
set -e

# Deploy monitor application to Kubernetes

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

ENV=${1:-dev}
NAMESPACE="device-monitor-$ENV"

if [[ ! "$ENV" =~ ^(dev|qa|prod)$ ]]; then
    echo "Usage: $0 [dev|qa|prod]"
    echo "Default: dev"
    exit 1
fi

echo "Deploying monitor application to $ENV environment..."

# Ensure namespace exists
kubectl create namespace "$NAMESPACE" --dry-run=client -o yaml | kubectl apply -f -

# Build and package if not already done
if [ ! -f "$PROJECT_ROOT/monitor/target/monitor-1.0.0.jar" ]; then
    echo "Building and packaging monitor application..."
    "$SCRIPT_DIR/package.sh"
fi

# Deploy Flink with monitor job
cd "$PROJECT_ROOT/infrastructure/flink"

echo "Installing/upgrading Flink deployment..."
helm upgrade --install monitor-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait

echo "Monitor application deployed successfully to $ENV!"
echo "Access Flink UI: kubectl port-forward -n $NAMESPACE svc/monitor-$ENV-jobmanager 8081:8081"
