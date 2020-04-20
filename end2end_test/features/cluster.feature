Feature: cluster

  Scenario: seeding one node
     Given configure seed nodes
        | node        | seeds        |
        | node_a      | node_a:4445  |
      When we start seed nodes
      Then it should join in one cluster

  Scenario: seeding two nodes
     Given configure seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
      When we start seed nodes
       And we wait 3 seconds
      Then they should join in one cluster

  Scenario: seeding multiple nodes when start one by one
     Given configure seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
       And node_a starts to create a new cluster
      When we wait 3 seconds
       And we start node_b
       And we wait 3 seconds
      Then they should join in one cluster

  Scenario: seeding multiple nodes when one seed restart
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
       And node_a turns down
      When we start node_a
       And we wait 3 seconds
      Then they should join in one cluster

  Scenario: joining
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
      When we add an ordinary node and start it
        | node        | seeds                   |
        | node_c      | node_a:4445,node_b:4445 |
       And we wait 3 seconds
      Then they should join in one cluster

  Scenario: joining when partial seeds down
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
        And node_a turns down
      When we add an ordinary node and start it
        | node        | seeds                   |
        | node_c      | node_a:4445,node_b:4445 |
       And we wait 3 seconds
      Then they should join in one cluster

  Scenario: joining when all seeds down
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
        And node_a turns down
        And node_b turns down
      When we add an ordinary node and start it
        | node        | seeds                   |
        | node_c      | node_a:4445,node_b:4445 |
       And we wait 3 seconds
      Then node_c should fail to join in existing cluster

  Scenario: leaving when node unreachable
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
        | node_c      | node_a:4445,node_b:4445 |
      When node_c turns unreachable
       And we wait 3 seconds
      Then node_c should be removed from cluster

  Scenario: leaving when node down
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
        | node_c      | node_a:4445,node_b:4445 |
      When node_c turns down
       And we wait 3 seconds
      Then node_c should be removed from cluster

  Scenario: rejoining
     Given cluster with multiple seed nodes
        | node        | seeds                   |
        | node_a      | node_a:4445,node_b:4445 |
        | node_b      | node_a:4445,node_b:4445 |
        | node_c      | node_a:4445,node_b:4445 |
       And node_c turns down
      When we wait 3 seconds
       And we start node_c
       And we wait 3 seconds
      Then they should join in one cluster
