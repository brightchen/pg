#!/bin/bash

# Script to uninstall Kafka cluster using Helm on Kubernetes
# Author: Qwen Code
# Date: October 7, 2025

set -e  # Exit immediately if a command exits with a non-zero status

# Variables
NAMESPACE="kafka"
RELEASE_NAME="kafka-cluster"

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

# Function to check if the Kafka release exists
check_release_exists() {
    print_msg "Checking if Kafka release exists..."
    if ! helm status $RELEASE_NAME -n $NAMESPACE &> /dev/null; then
        print_error "Kafka release '$RELEASE_NAME' does not exist in namespace '$NAMESPACE'."
        print_msg "Available releases in namespace $NAMESPACE:"
        helm list -n $NAMESPACE || true
        exit 1
    fi
}

# Function to uninstall Kafka using Helm
uninstall_kafka() {
    print_msg "Uninstalling Kafka cluster with Helm..."
    helm uninstall $RELEASE_NAME -n $NAMESPACE
    print_msg "Kafka cluster uninstalled successfully."
}

# Function to delete the namespace (optional)
delete_namespace_option() {
    read -p "Do you want to delete the namespace '$NAMESPACE' as well? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_msg "Deleting namespace: $NAMESPACE"
        kubectl delete namespace $NAMESPACE
        print_msg "Namespace '$NAMESPACE' deleted."
    else
        print_msg "Keeping namespace '$NAMESPACE'."
        print_msg "To delete it manually later, run: kubectl delete namespace $NAMESPACE"
    fi
}

# Function to show remaining resources in namespace
show_remaining_resources() {
    print_msg "Checking for remaining resources in namespace: $NAMESPACE"
    if kubectl get all -n $NAMESPACE &> /dev/null; then
        kubectl get all -n $NAMESPACE
    else
        print_msg "Namespace $NAMESPACE does not exist or has been deleted."
    fi
}

# Main execution
main() {
    print_msg "Starting Kafka cluster uninstallation..."
    
    check_kubectl
    check_helm
    check_release_exists
    uninstall_kafka
    delete_namespace_option
    show_remaining_resources
    
    print_msg "Kafka cluster uninstallation completed!"
}

# Run main function
main