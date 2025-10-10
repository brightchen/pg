#!/bin/bash

# Script to install Kafka cluster using Helm on Kubernetes
# Author: Qwen Code
# Date: October 7, 2025

set -e  # Exit immediately if a command exits with a non-zero status

# Variables
NAMESPACE="kafka"
RELEASE_NAME="kafka-cluster"
TIMEOUT="3m"

# Function to print messages
print_msg() {
    echo -e "\033[1;34m$1\033[0m"
}

# Function to print error messages
print_error() {
    echo -e "\033[1;31m$1\033[0m"
}

# Function to check if kubectl is installed
check_kubectl() {
    if ! command -v kubectl &> /dev/null; then
        print_error "kubectl is not installed. Please install kubectl first."
        exit 1
    fi
}

# Function to check if helm is installed
check_helm() {
    if ! command -v helm &> /dev/null; then
        print_error "helm is not installed. Please install helm first."
        exit 1
    fi
}

# Function to check if a Kubernetes cluster is available
check_k8s_cluster() {
    print_msg "Checking if Kubernetes cluster is available..."
    if ! kubectl cluster-info &> /dev/null; then
        print_error "Kubernetes cluster is not available. Please ensure your KUBECONFIG is set correctly."
        exit 1
    fi
}

# Function to create namespace if it doesn't exist
create_namespace() {
    print_msg "Creating namespace: $NAMESPACE"
    if ! kubectl get namespace $NAMESPACE &> /dev/null; then
        kubectl create namespace $NAMESPACE
    else
        print_msg "Namespace $NAMESPACE already exists"
    fi
}

# Function to add the Bitnami Helm repository
add_helm_repo() {
    print_msg "Adding Bitnami Helm repository..."
    helm repo add bitnami https://charts.bitnami.com/bitnami
    helm repo update
}

# Function to install Kafka using Helm
install_kafka() {
    print_msg "Installing Kafka cluster with Helm..."
    
    # Install Kafka with default values, can be customized as needed
    # This command timeout due to error(kc describe pod kafka-cluster-controller-0 -n kafk):
    #  Failed to pull image "docker.io/bitnami/kafka:4.0.0-debian-12-r10": Error response from daemon: manifest for bitnami/kafka:4.0.0-debian-12-r10 not found: manifest unknown: manifest unknown
    # not work: set image.tag="latest"
    # works: docker pull apache/kafka:4.1.0 
    helm install $RELEASE_NAME bitnami/kafka \
        --namespace $NAMESPACE \
        --set replicaCount=1 \
        --set autoCreateTopicsEnable=true \
        --set persistence.enabled=true \
        --set persistence.size=2Gi \
        --set image.tag="32.4.3" \
        --timeout $TIMEOUT

    print_msg "Kafka cluster installation initiated. Waiting for pods to be ready..."
}

# Function to wait for Kafka pods to be ready
wait_for_pods() {
    print_msg "Waiting for Kafka pods to be ready (timeout: $TIMEOUT)..."
    kubectl wait --for=condition=ready pod -l app.kubernetes.io/name=kafka -n $NAMESPACE --timeout=$TIMEOUT
}

# Function to get Kafka connection information
get_connection_info() {
    print_msg "Getting Kafka connection information..."
    
    # Get external IP/LoadBalancer if available, otherwise use ClusterIP
    if kubectl get svc -n $NAMESPACE | grep -q LoadBalancer; then
        EXTERNAL_IP=$(kubectl get svc -n $NAMESPACE -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
        if [ -z "$EXTERNAL_IP" ]; then
            EXTERNAL_IP=$(kubectl get svc -n $NAMESPACE -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
        fi
        print_msg "Kafka is accessible externally at: $EXTERNAL_IP:9092"
    else
        SERVICE_IP=$(kubectl get svc -n $NAMESPACE -o jsonpath='{.spec.clusterIP}')
        print_msg "Kafka is accessible internally at: $SERVICE_IP:9092"
        print_msg "To connect from within the cluster: $RELEASE_NAME-kafka.$NAMESPACE.svc.cluster.local:9092"
    fi
    
    print_msg "To port forward locally for testing: kubectl port-forward -n $NAMESPACE svc/$RELEASE_NAME-kafka 9092:9092"
}

# Function to show Kafka status
show_status() {
    print_msg "Kafka cluster status:"
    kubectl get pods,svc -n $NAMESPACE
}

# Main execution
main() {
    print_msg "Starting Kafka cluster installation using Helm..."
    
    check_kubectl
    check_helm
    check_k8s_cluster
    create_namespace
    add_helm_repo
    install_kafka
    wait_for_pods
    get_connection_info
    show_status
    
    print_msg "Kafka cluster installation completed successfully!"
    print_msg "Next steps:"
    echo "1. Port forward to access Kafka locally: kubectl port-forward -n $NAMESPACE svc/$RELEASE_NAME-kafka 9092:9092"
    echo "2. Test the connection using Kafka client tools"
    echo "3. Check logs: kubectl logs -n $NAMESPACE -l app.kubernetes.io/name=kafka"
}

# Run main function
main