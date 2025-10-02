#!/bin/bash
set -e

# Stop/undeploy monitoring stack from Kubernetes

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
echo "Stopping Monitoring Stack in $ENV"
echo "========================================="
echo ""
echo "WARNING: This will remove all monitoring components."
echo "Data in PersistentVolumes will be preserved unless you delete them manually."
echo ""
read -p "Are you sure you want to continue? (yes/no): " confirm

if [[ "$confirm" != "yes" ]]; then
    echo "Cancelled."
    exit 0
fi

echo ""
echo "Uninstalling Grafana..."
helm uninstall grafana-$ENV --namespace "$NAMESPACE" 2>/dev/null || echo "Grafana not found"

echo ""
echo "Uninstalling Vector..."
helm uninstall vector-$ENV --namespace "$NAMESPACE" 2>/dev/null || echo "Vector not found"

echo ""
echo "Uninstalling TimescaleDB..."
helm uninstall timescaledb-$ENV --namespace "$NAMESPACE" 2>/dev/null || echo "TimescaleDB not found"

echo ""
echo "Deleting dashboard ConfigMap..."
kubectl delete configmap grafana-$ENV-dashboards --namespace "$NAMESPACE" 2>/dev/null || echo "ConfigMap not found"

echo ""
echo "========================================="
echo "Monitoring Stack Stopped"
echo "========================================="
echo ""
echo "Note: PersistentVolumeClaims were NOT deleted to preserve data."
echo ""
echo "To view remaining PVCs:"
echo "kubectl get pvc -n $NAMESPACE"
echo ""
echo "To delete PVCs and all data (IRREVERSIBLE):"
echo "kubectl delete pvc -n $NAMESPACE --all"
echo ""
