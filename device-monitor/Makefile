.PHONY: help build test integration-test package clean deploy-dev deploy-qa deploy-prod deploy-all-dev deploy-all-qa deploy-all-prod docker-up docker-down run-generator

help:
	@echo "IoT Device Monitor - Available commands:"
	@echo ""
	@echo "Build & Test:"
	@echo "  make build              - Build all applications"
	@echo "  make test               - Run unit tests"
	@echo "  make integration-test   - Run integration tests (requires Docker)"
	@echo "  make package            - Package applications as JAR files"
	@echo "  make clean              - Clean build artifacts"
	@echo ""
	@echo "Deploy Monitor Application:"
	@echo "  make deploy-dev         - Deploy monitor to dev environment"
	@echo "  make deploy-qa          - Deploy monitor to qa environment"
	@echo "  make deploy-prod        - Deploy monitor to prod environment"
	@echo ""
	@echo "Deploy Full System:"
	@echo "  make deploy-all-dev     - Deploy Kafka + Monitor to dev"
	@echo "  make deploy-all-qa      - Deploy Kafka + Monitor to qa"
	@echo "  make deploy-all-prod    - Deploy Kafka + Monitor to prod"
	@echo ""
	@echo "Local Development:"
	@echo "  make docker-up          - Start local Kafka + Flink with Docker Compose"
	@echo "  make docker-down        - Stop local Docker Compose environment"
	@echo "  make run-generator      - Run event generator locally"

build:
	@./scripts/build.sh

test:
	@./scripts/test.sh

integration-test:
	@./scripts/integration-test.sh

package:
	@./scripts/package.sh

clean:
	@cd monitor && mvn clean
	@cd event-generator && mvn clean
	@rm -rf monitor/target event-generator/target

deploy-dev:
	@./scripts/deploy-monitor.sh dev

deploy-qa:
	@./scripts/deploy-monitor.sh qa

deploy-prod:
	@./scripts/deploy-monitor.sh prod

deploy-all-dev:
	@./scripts/deploy-all.sh dev

deploy-all-qa:
	@./scripts/deploy-all.sh qa

deploy-all-prod:
	@./scripts/deploy-all.sh prod

docker-up: package
	@docker compose up -d
	@echo "Waiting for services to start..."
	@sleep 10
	@echo "Services started!"
	@echo "Flink UI: http://localhost:8081"
	@echo "Kafka: localhost:9094"

docker-down:
	@docker compose down -v

run-generator:
	@./scripts/run-event-generator.sh
