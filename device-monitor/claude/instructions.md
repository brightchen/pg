- please generate resource based on CLAUDE.md file
- The AlertAggregator extends ProcessWindowFunction, but used less Generic parameters, The suppose have 4
parameters: ProcessWindowFunction<IN, OUT, KEY, W extends Window>. Could you fix this issue and make sure source
code compilable
-  The integration test didn't check if the kafka cluster running or not, could you please integrate the integrate
  test case with the kafka deployment, and deploy and start the kafka cluser if it haven't started yet. And then
  verify the test topic existed or not, and create test topic if it hasn't created. cleanup the messages in kafka
  test topic if it already have messages. No need to delete test topic and shutdown kafka cluster after integration
  test done
- There are some error when run integration test, could you please fix it
- Now the alerts are generated and send to kafka topic. Next step I want to display these alerts. I want to use
  datadog's vector to read alerts from kafka topic, then convert and and write to the alerts table of TimescaleDB,
  the use Grafana to display the alerts. Could you please config and deploy the datadog vector, TimescaleDB and
  Grafana? And all these components are managed by k8s. Also provide entry point run and stop command to start all
  these components. And also please update the readme.md

