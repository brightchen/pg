package bc.monitor;

import org.apache.kafka.clients.admin.*;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.consumer.OffsetAndMetadata;
import org.apache.kafka.common.TopicPartition;
import org.apache.kafka.common.serialization.StringDeserializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

/**
 * Helper class to manage Kafka cluster for integration tests.
 * Checks if Kafka is running, starts it if needed, and manages test topics.
 */
public class KafkaTestHelper {

    private static final Logger LOG = LoggerFactory.getLogger(KafkaTestHelper.class);
    private static final String DEFAULT_BOOTSTRAP_SERVERS = "localhost:9094";
    private static final int KAFKA_CONNECTION_TIMEOUT_MS = 5000;

    private final String bootstrapServers;
    private AdminClient adminClient;
    private boolean kafkaWasStarted = false;

    public KafkaTestHelper() {
        this.bootstrapServers = System.getProperty("kafka.bootstrap.servers",
                System.getenv().getOrDefault("KAFKA_BOOTSTRAP_SERVERS", DEFAULT_BOOTSTRAP_SERVERS));
    }

    /**
     * Ensure Kafka cluster is running and ready for tests.
     */
    public void ensureKafkaRunning() throws Exception {
        LOG.info("Checking if Kafka is running at {}...", bootstrapServers);

        if (!isKafkaRunning()) {
            LOG.info("Kafka is not running. Starting Kafka with docker-compose...");
            startKafkaWithDockerCompose();
            kafkaWasStarted = true;

            // Wait for Kafka to be ready
            int maxRetries = 30;
            int retries = 0;
            while (!isKafkaRunning() && retries < maxRetries) {
                LOG.info("Waiting for Kafka to be ready... ({}/{})", retries + 1, maxRetries);
                Thread.sleep(2000);
                retries++;
            }

            if (!isKafkaRunning()) {
                throw new RuntimeException("Failed to start Kafka after " + maxRetries + " retries");
            }
        }

        LOG.info("Kafka is running at {}", bootstrapServers);

        // Create admin client
        Properties adminProps = new Properties();
        adminProps.put(AdminClientConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        adminProps.put(AdminClientConfig.REQUEST_TIMEOUT_MS_CONFIG, KAFKA_CONNECTION_TIMEOUT_MS);
        adminClient = AdminClient.create(adminProps);
    }

    /**
     * Ensure test topics exist and are clean.
     */
    public void ensureTopicsExistAndClean(String... topicNames) throws Exception {
        LOG.info("Ensuring topics exist and are clean: {}", Arrays.toString(topicNames));

        Set<String> existingTopics = adminClient.listTopics().names().get();
        List<NewTopic> topicsToCreate = new ArrayList<>();

        for (String topicName : topicNames) {
            if (!existingTopics.contains(topicName)) {
                LOG.info("Topic '{}' does not exist. Creating...", topicName);
                int partitions = topicName.equals("events") ? 3 : 1;
                topicsToCreate.add(new NewTopic(topicName, partitions, (short) 1));
            } else {
                LOG.info("Topic '{}' already exists", topicName);
            }
        }

        if (!topicsToCreate.isEmpty()) {
            adminClient.createTopics(topicsToCreate).all().get();
            LOG.info("Created {} topics", topicsToCreate.size());
        }

        // Clean up topics by deleting records (Kafka 0.11+)
        for (String topicName : topicNames) {
            cleanupTopic(topicName);
        }

        LOG.info("Topics are ready for testing");
    }

    /**
     * Clean up messages in a topic by deleting all records.
     */
    private void cleanupTopic(String topicName) throws Exception {
        LOG.info("Cleaning up topic '{}'...", topicName);

        // Get topic partitions
        Map<String, TopicDescription> descriptions = adminClient.describeTopics(
                Collections.singletonList(topicName)).allTopicNames().get();

        TopicDescription description = descriptions.get(topicName);
        if (description == null) {
            LOG.warn("Topic '{}' not found, skipping cleanup", topicName);
            return;
        }

        // Create consumer to get end offsets
        Properties consumerProps = new Properties();
        consumerProps.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
        consumerProps.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class.getName());
        consumerProps.put(ConsumerConfig.GROUP_ID_CONFIG, "test-cleanup-" + UUID.randomUUID());

        try (KafkaConsumer<String, String> consumer = new KafkaConsumer<>(consumerProps)) {
            List<TopicPartition> partitions = description.partitions().stream()
                    .map(p -> new TopicPartition(topicName, p.partition()))
                    .collect(Collectors.toList());

            Map<TopicPartition, Long> endOffsets = consumer.endOffsets(partitions);

            // Delete records up to end offset for each partition
            Map<TopicPartition, RecordsToDelete> recordsToDelete = new HashMap<>();
            for (Map.Entry<TopicPartition, Long> entry : endOffsets.entrySet()) {
                if (entry.getValue() > 0) {
                    recordsToDelete.put(entry.getKey(), RecordsToDelete.beforeOffset(entry.getValue()));
                    LOG.info("  Partition {}: deleting {} records", entry.getKey().partition(), entry.getValue());
                }
            }

            if (!recordsToDelete.isEmpty()) {
                DeleteRecordsResult result = adminClient.deleteRecords(recordsToDelete);
                result.all().get();
                LOG.info("Cleaned up topic '{}'", topicName);
            } else {
                LOG.info("Topic '{}' is already clean", topicName);
            }
        }
    }

    /**
     * Check if Kafka is running and accessible.
     */
    private boolean isKafkaRunning() {
        try {
            Properties props = new Properties();
            props.put(AdminClientConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrapServers);
            props.put(AdminClientConfig.REQUEST_TIMEOUT_MS_CONFIG, KAFKA_CONNECTION_TIMEOUT_MS);
            props.put(AdminClientConfig.DEFAULT_API_TIMEOUT_MS_CONFIG, KAFKA_CONNECTION_TIMEOUT_MS);

            try (AdminClient testClient = AdminClient.create(props)) {
                testClient.listTopics().names().get();
                return true;
            }
        } catch (Exception e) {
            LOG.debug("Kafka is not accessible: {}", e.getMessage());
            return false;
        }
    }

    /**
     * Start Kafka using docker-compose.
     */
    private void startKafkaWithDockerCompose() throws IOException, InterruptedException {
        // Find project root (go up from monitor directory)
        String projectRoot = System.getProperty("user.dir");
        if (projectRoot.endsWith("/monitor")) {
            projectRoot = projectRoot.substring(0, projectRoot.length() - "/monitor".length());
        }

        LOG.info("Starting Kafka with docker compose from: {}", projectRoot);

        // Verify docker-compose.yaml exists
        java.io.File dockerComposeFile = new java.io.File(projectRoot, "docker-compose.yaml");
        if (!dockerComposeFile.exists()) {
            throw new RuntimeException("docker-compose.yaml not found at: " + dockerComposeFile.getAbsolutePath());
        }

        ProcessBuilder pb = new ProcessBuilder(
                "docker", "compose", "up", "-d", "kafka", "kafka-setup"
        );
        pb.directory(new java.io.File(projectRoot));
        pb.redirectErrorStream(true);

        Process process = pb.start();

        // Capture output
        StringBuilder output = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                LOG.info("docker compose: {}", line);
                output.append(line).append("\n");
            }
        }

        int exitCode = process.waitFor();
        if (exitCode != 0) {
            String errorMsg = String.format(
                "Failed to start Kafka with docker compose. Exit code: %d\nWorking directory: %s\nOutput:\n%s",
                exitCode, projectRoot, output.toString()
            );
            LOG.error(errorMsg);
            throw new RuntimeException(errorMsg);
        }

        LOG.info("Docker compose started successfully");
    }

    /**
     * Check if this helper started Kafka.
     */
    public boolean wasKafkaStarted() {
        return kafkaWasStarted;
    }

    /**
     * Get bootstrap servers.
     */
    public String getBootstrapServers() {
        return bootstrapServers;
    }

    /**
     * Close resources.
     */
    public void close() {
        if (adminClient != null) {
            adminClient.close(Duration.ofSeconds(5));
        }
    }
}
