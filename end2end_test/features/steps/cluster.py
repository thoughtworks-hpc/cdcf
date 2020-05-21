from behave import *
import time
import service


@given('configure seed nodes')
def step_impl(context):
    context.nodes = {}
    for row in context.table:
        node = service.configure_node(row['node'], row['seeds'])
        context.nodes[row['node']] = node


@when('we start seed nodes')
def step_impl(context):
    for name, node in context.nodes.items():
        node.run()


@then('{_} should join in one cluster')
def step_impl(context, _):
    running_nodes = [node for name, node in context.nodes.items() if node.running()]
    running_ids = [service.Node.id(node.name) for node in running_nodes]
    for node in running_nodes:
        members = node.members()
        assert all(
            node_id in members for node_id in running_ids), f"{members} should include all running ids {running_ids}"


@then('{node} should fail to join in existing cluster')
def step_impl(context, node):
    assert context.nodes[node].members() == [service.Node.id(node)]


@then('{removed_name} should be removed from cluster')
def step_impl(context, removed_name):
    removed_id = service.Node.id(removed_name)
    for name, node in context.nodes.items():
        if name != removed_name:
            members = node.members()
            assert removed_id not in members, f"{members} should not include {removed_id}"


@given('{node} starts to create a new cluster')
def step_impl(context, node):
    context.nodes[node].run()


@when('we start {node}')
def step_impl(context, node):
    context.nodes[node].start()


@when('we wait {seconds} seconds')
def step_impl(context, seconds):
    time.sleep(int(seconds))


@given('cluster with multiple seed nodes')
def step_impl(context):
    context.nodes = {}
    for row in context.table:
        node = service.configure_node(row['node'], row['seeds'])
        context.nodes[row['node']] = node
    for name, node in context.nodes.items():
        node.run()


@given('{node} turns down')
def step_impl(context, node):
    context.nodes[node].stop()


@when('{node} turns down')
def step_impl(context, node):
    context.nodes[node].stop()


@when('{node} turns unreachable')
def step_impl(context, node):
    context.nodes[node].disconnect()


@when('we add an ordinary node and start it')
def step_impl(context):
    for row in context.table:
        node = service.configure_node(row['node'], row['seeds'])
        context.nodes[row['node']] = node
        print("add: ", row['node'])
        node.run()
