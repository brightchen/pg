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

