#!/bin/bash
set -e

# Deploy monitoring stack (TimescaleDB + Vector + Grafana) to Kubernetes

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
echo "Deploying Monitoring Stack to $ENV"
echo "========================================="

# Ensure namespace exists
echo "Creating namespace $NAMESPACE..."
kubectl create namespace "$NAMESPACE" --dry-run=client -o yaml | kubectl apply -f -

# Add Grafana Helm repository
echo ""
echo "Adding Grafana Helm repository..."
helm repo add grafana https://grafana.github.io/helm-charts 2>/dev/null || true
helm repo update

# Deploy TimescaleDB
echo ""
echo "========================================="
echo "Deploying TimescaleDB..."
echo "========================================="
cd "$PROJECT_ROOT/infrastructure/timescaledb"

helm upgrade --install timescaledb-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait \
    --timeout 10m

echo "TimescaleDB deployed successfully!"

# Wait for TimescaleDB to be ready
echo "Waiting for TimescaleDB to be fully ready..."
kubectl wait --for=condition=ready pod -l app=timescaledb -n "$NAMESPACE" --timeout=300s

# Deploy Vector
echo ""
echo "========================================="
echo "Deploying Vector..."
echo "========================================="
cd "$PROJECT_ROOT/infrastructure/vector"

helm upgrade --install vector-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait \
    --timeout 10m

echo "Vector deployed successfully!"

# Deploy Grafana
echo ""
echo "========================================="
echo "Deploying Grafana..."
echo "========================================="
cd "$PROJECT_ROOT/infrastructure/grafana"

# First create the dashboard ConfigMap
kubectl create configmap grafana-$ENV-dashboards \
    --from-file=dashboards/ \
    --namespace "$NAMESPACE" \
    --dry-run=client -o yaml | kubectl apply -f -

helm upgrade --install grafana-$ENV . \
    --namespace "$NAMESPACE" \
    --values "values-$ENV.yaml" \
    --wait \
    --timeout 10m

echo "Grafana deployed successfully!"

echo ""
echo "========================================="
echo "Monitoring Stack Deployment Complete!"
echo "========================================="
echo ""
echo "Environment: $ENV"
echo "Namespace: $NAMESPACE"
echo ""
echo "Components deployed:"
echo "  - TimescaleDB: timescaledb-$ENV-timescaledb"
echo "  - Vector: vector-$ENV-vector"
echo "  - Grafana: grafana-$ENV"
echo ""
echo "Useful commands:"
echo ""
echo "# Check pod status"
echo "kubectl get pods -n $NAMESPACE"
echo ""
echo "# View TimescaleDB logs"
echo "kubectl logs -n $NAMESPACE -l app=timescaledb"
echo ""
echo "# View Vector logs"
echo "kubectl logs -n $NAMESPACE -l app=vector"
echo ""
echo "# View Grafana logs"
echo "kubectl logs -n $NAMESPACE -l app.kubernetes.io/name=grafana"
echo ""
echo "# Access Grafana UI (port-forward)"
echo "kubectl port-forward -n $NAMESPACE svc/grafana-$ENV 3000:80"
echo "# Then open http://localhost:3000"
echo "# Username: admin"
echo "# Password: (check values-$ENV.yaml)"
echo ""
echo "# Connect to TimescaleDB"
echo "kubectl port-forward -n $NAMESPACE svc/timescaledb-$ENV-timescaledb 5432:5432"
echo "# Then: psql -h localhost -U monitor_user -d monitoring"
echo ""
