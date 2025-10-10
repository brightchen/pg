#!/bin/bash

# Script to manage Kafka operations (status, topics, publish/consume)
# Author: Qwen Code
# Date: October 7, 2025

set -e  # Exit immediately if a command exits with a non-zero status

# Variables
NAMESPACE="kafka"
RELEASE_NAME="kafka-cluster"
BROKER_LIST="$RELEASE_NAME-kafka-headless.$NAMESPACE.svc.cluster.local:9092"

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

# Function to check if Kafka is running
check_kafka_status() {
    print_msg "Checking Kafka cluster status..."
    if kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=kafka -l app.kubernetes.io/instance=$RELEASE_NAME &> /dev/null; then
        kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=kafka -l app.kubernetes.io/instance=$RELEASE_NAME
    else
        print_error "No Kafka pods found in namespace $NAMESPACE"
        exit 1
    fi
}

# Function to list Kafka brokers
list_brokers() {
    print_msg "Listing Kafka brokers..."
    print_msg "Broker connection string: $BROKER_LIST"
    print_msg "Note: Brokers may also be accessible via individual pod names if needed"
    
    # Try to get broker information if possible
    if kubectl get svc -n $NAMESPACE $RELEASE_NAME-kafka-headless &> /dev/null; then
        kubectl get svc -n $NAMESPACE $RELEASE_NAME-kafka-headless -o wide
    else
        print_error "Kafka headless service not found"
        exit 1
    fi
}

# Function to list Kafka topics
list_topics() {
    print_msg "Listing Kafka topics..."
    kubectl exec -n $NAMESPACE $RELEASE_NAME-kafka-0 -- \
        kafka-topics.sh --list --bootstrap-server localhost:9092
}

# Function to create a Kafka topic
create_topic() {
    if [ $# -ne 1 ]; then
        print_error "Usage: $0 create-topic <topic-name>"
        exit 1
    fi
    
    TOPIC_NAME=$1
    print_msg "Creating Kafka topic: $TOPIC_NAME"
    
    kubectl exec -n $NAMESPACE $RELEASE_NAME-kafka-0 -- \
        kafka-topics.sh --create --topic $TOPIC_NAME \
        --bootstrap-server localhost:9092 \
        --partitions 1 \
        --replication-factor 1
}

# Function to publish data to a topic
publish_to_topic() {
    if [ $# -ne 2 ]; then
        print_error "Usage: $0 publish <topic-name> <message>"
        exit 1
    fi
    
    TOPIC_NAME=$1
    MESSAGE=$2
    
    print_msg "Publishing message to topic: $TOPIC_NAME"
    
    # Use kubectl exec to run the console producer
    echo "$MESSAGE" | kubectl exec -n $NAMESPACE -i $RELEASE_NAME-kafka-0 -- \
        kafka-console-producer.sh --topic $TOPIC_NAME \
        --bootstrap-server localhost:9092
}

# Function to consume data from a topic
consume_from_topic() {
    if [ $# -ne 1 ]; then
        print_error "Usage: $0 consume <topic-name>"
        exit 1
    fi
    
    TOPIC_NAME=$1
    
    print_msg "Consuming messages from topic: $TOPIC_NAME (Press Ctrl+C to stop)"
    
    # Use kubectl exec to run the console consumer
    kubectl exec -n $NAMESPACE -i $RELEASE_NAME-kafka-0 -- \
        kafka-console-consumer.sh --topic $TOPIC_NAME \
        --bootstrap-server localhost:9092 --from-beginning
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [command] [args...]"
    echo ""
    echo "Commands:"
    echo "  status                    - Check Kafka cluster status"
    echo "  list-brokers             - List Kafka brokers"
    echo "  list-topics              - List all Kafka topics"
    echo "  create-topic <topic>     - Create a new Kafka topic"
    echo "  publish <topic> <msg>    - Publish a message to a topic"
    echo "  consume <topic>          - Consume messages from a topic"
    echo ""
    echo "Example:"
    echo "  $0 status"
    echo "  $0 list-topics"
    echo "  $0 create-topic my-topic"
    echo "  $0 publish my-topic 'Hello Kafka!'"
    echo "  $0 consume my-topic"
}

# Main execution
main() {
    check_kubectl
    
    if [ $# -eq 0 ]; then
        show_usage
        exit 1
    fi
    
    COMMAND=$1
    shift  # Remove the command from the arguments list
    
    case $COMMAND in
        "status")
            check_kafka_status
            ;;
        "list-brokers")
            list_brokers
            ;;
        "list-topics")
            list_topics
            ;;
        "create-topic")
            create_topic "$@"
            ;;
        "publish")
            publish_to_topic "$@"
            ;;
        "consume")
            consume_from_topic "$@"
            ;;
        *)
            print_error "Unknown command: $COMMAND"
            show_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"