# Kafka Cluster Installation with Helm

This repository contains scripts and documentation to install a Kafka cluster on Kubernetes using Helm.

## Prerequisites

Before running the installation script, ensure you have the following prerequisites installed:

- **kubectl** - Kubernetes command-line tool
- **Helm** - Package manager for Kubernetes
- **Kubernetes cluster** - A running Kubernetes cluster (e.g., Minikube, Kind, EKS, GKE, AKS)

## Directory Structure

```
kafka/
├── scripts/
│   ├── install-kafka.sh          # Installation script
│   ├── uninstall-kafka.sh        # Uninstallation script
│   └── kafka-operations.sh       # Kafka operations script (status, topics, publish/consume)
└── README.md                     # This file
```

## Installation

### Quick Install

1. Clone or navigate to this directory
2. Run the installation script:

```bash
chmod +x scripts/install-kafka.sh
./scripts/install-kafka.sh
```

### Custom Configuration

The installation script installs Kafka with the following default configuration:

- 1 Kafka replica
- Auto topic creation enabled
- Persistent storage of 2Gi per broker
- Namespace: `kafka`
- Release name: `kafka-cluster`

To customize the installation, you can modify the script or create a `values.yaml` file with your configuration and modify the script to use it.

## Accessing Kafka

After installation, you can access Kafka in different ways:

### Within the Cluster

If your applications are running in the same Kubernetes cluster:

```
kafka-cluster-kafka.kafka.svc.cluster.local:9092
```

### Locally (for testing)

Port forward to access Kafka locally:

```bash
kubectl port-forward -n kafka svc/kafka-cluster-kafka 9092:9092
```

Then connect your Kafka client to `localhost:9092`.

## Verifying Installation

Check if the Kafka pods are running:

```bash
kubectl get pods -n kafka
```

Check Kafka service:

```bash
kubectl get svc -n kafka
```

View logs:

```bash
kubectl logs -n kafka -l app.kubernetes.io/name=kafka
```

## Configuration Parameters

The script installs the Bitnami Kafka Helm chart with the following default settings:

- `replicaCount`: 1
- `autoCreateTopicsEnable`: true (topics are created automatically)
- `persistence.enabled`: true (persistent storage enabled)
- `persistence.size`: 2Gi (storage size per broker)

## Troubleshooting

### Common Issues

1. **Timeout during installation**: Ensure your Kubernetes cluster has enough resources (CPU, memory) to run Kafka.

2. **Storage issues**: If using a local cluster like Minikube, ensure you have a default storage class configured.

3. **Network connectivity**: If Kafka pods are not becoming ready, check your cluster's network configuration.

### Useful Commands

```bash
# Check all resources in the kafka namespace
kubectl get all -n kafka

# Describe a specific pod for more details
kubectl describe pod -l app.kubernetes.io/name=kafka -n kafka

# View logs for all Kafka pods
kubectl logs -l app.kubernetes.io/name=kafka -n kafka

# Check Kafka cluster status
helm status kafka-cluster -n kafka
```

## Kafka Operations

After installing Kafka, you can perform various operations using the operations script:

```bash
./scripts/kafka-operations.sh [command] [args...]
```

### Available Commands

#### Check Kafka Status
```bash
./scripts/kafka-operations.sh status
```
Shows the status of Kafka pods in the cluster.

#### List Kafka Brokers
```bash
./scripts/kafka-operations.sh list-brokers
```
Lists the Kafka brokers and their connection information.

#### List Kafka Topics
```bash
./scripts/kafka-operations.sh list-topics
```
Lists all existing Kafka topics.

#### Create a Kafka Topic
```bash
./scripts/kafka-operations.sh create-topic <topic-name>
```
Creates a new Kafka topic with the specified name.

Example:
```bash
./scripts/kafka-operations.sh create-topic my-test-topic
```

#### Publish Data to a Topic
```bash
./scripts/kafka-operations.sh publish <topic-name> <message>
```
Publishes a message to the specified topic.

Example:
```bash
./scripts/kafka-operations.sh publish my-test-topic "Hello Kafka!"
```

#### Consume Data from a Topic
```bash
./scripts/kafka-operations.sh consume <topic-name>
```
Consumes messages from the specified topic (starts from the beginning).

Example:
```bash
./scripts/kafka-operations.sh consume my-test-topic
```
Press `Ctrl+C` to stop consuming.

## Uninstalling

### Using the Uninstall Script (Recommended)

Run the uninstall script to remove the Kafka cluster:

```bash
./scripts/uninstall-kafka.sh
```

The script will:
1. Check if the Kafka release exists
2. Uninstall the Kafka cluster using Helm
3. Optionally delete the namespace
4. Show any remaining resources

### Manual Uninstallation

To manually remove the Kafka cluster, run:

```bash
helm uninstall kafka-cluster -n kafka
kubectl delete namespace kafka
```

## Additional Resources

- [Bitnami Kafka Chart Documentation](https://github.com/bitnami/charts/tree/main/bitnami/kafka)
- [Kafka Documentation](https://kafka.apache.org/documentation/)
- [Helm Documentation](https://helm.sh/docs/)
- [Kubernetes Documentation](https://kubernetes.io/docs/home/)