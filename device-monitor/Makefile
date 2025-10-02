.PHONY: help build test integration-test package clean deploy-dev deploy-qa deploy-prod deploy-all-dev deploy-all-qa deploy-all-prod deploy-monitoring-dev deploy-monitoring-qa deploy-monitoring-prod stop-monitoring-dev stop-monitoring-qa stop-monitoring-prod deploy-full-dev deploy-full-qa deploy-full-prod stop-full-dev stop-full-qa stop-full-prod docker-up docker-down run-generator

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
	@echo "Deploy Kafka + Monitor:"
	@echo "  make deploy-all-dev     - Deploy Kafka + Monitor to dev"
	@echo "  make deploy-all-qa      - Deploy Kafka + Monitor to qa"
	@echo "  make deploy-all-prod    - Deploy Kafka + Monitor to prod"
	@echo ""
	@echo "Deploy Monitoring Stack (TimescaleDB + Vector + Grafana):"
	@echo "  make deploy-monitoring-dev   - Deploy monitoring stack to dev"
	@echo "  make deploy-monitoring-qa    - Deploy monitoring stack to qa"
	@echo "  make deploy-monitoring-prod  - Deploy monitoring stack to prod"
	@echo ""
	@echo "Deploy Complete System (Everything):"
	@echo "  make deploy-full-dev    - Deploy complete system to dev"
	@echo "  make deploy-full-qa     - Deploy complete system to qa"
	@echo "  make deploy-full-prod   - Deploy complete system to prod"
	@echo ""
	@echo "Stop Services:"
	@echo "  make stop-monitoring-dev     - Stop monitoring stack in dev"
	@echo "  make stop-monitoring-qa      - Stop monitoring stack in qa"
	@echo "  make stop-monitoring-prod    - Stop monitoring stack in prod"
	@echo "  make stop-full-dev           - Stop complete system in dev"
	@echo "  make stop-full-qa            - Stop complete system in qa"
	@echo "  make stop-full-prod          - Stop complete system in prod"
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

deploy-monitoring-dev:
	@./scripts/deploy-monitoring.sh dev

deploy-monitoring-qa:
	@./scripts/deploy-monitoring.sh qa

deploy-monitoring-prod:
	@./scripts/deploy-monitoring.sh prod

stop-monitoring-dev:
	@./scripts/stop-monitoring.sh dev

stop-monitoring-qa:
	@./scripts/stop-monitoring.sh qa

stop-monitoring-prod:
	@./scripts/stop-monitoring.sh prod

deploy-full-dev:
	@./scripts/deploy-full-stack.sh dev

deploy-full-qa:
	@./scripts/deploy-full-stack.sh qa

deploy-full-prod:
	@./scripts/deploy-full-stack.sh prod

stop-full-dev:
	@./scripts/stop-full-stack.sh dev

stop-full-qa:
	@./scripts/stop-full-stack.sh qa

stop-full-prod:
	@./scripts/stop-full-stack.sh prod

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
