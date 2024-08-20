# Eleminima: declarative graphics compositing language

## Grammar definition

### Node
A `node` is the data type used in this language and can be one of many possible subtypes. 

The syntax to define a node is as follows:
```
Identifier = Node::*1
{
    Node::*1::opt_*... 
};
```

Less cryptically
```
(name) = (node type) 
{
    (options for node) 
};
```

### Edge
Edges connect output of nodes to inputs of other nodes. 

The syntax is as follows: 
```
id_1 Node::*1::out_* -> id_2 Node::*2::in_*;
```
where
```
id_1 = Node::*1 
{
    opt_*
};

id_2 = Node::*2 
{
    opt_*
};
```
has been defined earlier.

Less cryptically
```
(node_1 name) (node 1 output selector) 
-> (node 2 name) (node 2 input selector);
```

If `node2` is not defined, the edge will be ignored. 

## Behavior definition

1. Nodes must be defined before they are connected by edges for the connections to be valid. Otherwise, the edge will be ignored. 

    This behavior is subject to change in future releases.

1. Input edges should be unique to the input slot for each node. If there are multiple input edges to an input slot for a particular node, later edges will overwrite earlier edges. 

## I/O

There may be multiple input or output images. 

## Nodes and options
coming soon~
