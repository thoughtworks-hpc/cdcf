def after_scenario(context, scenario):
    for name, node in context.nodes.items():
        try:
            # if context.failed:
            # node.log()
            node.stop()
            node.remove()
        except Exception as e:
            print(e)
