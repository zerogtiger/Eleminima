# Eleminima: declarative graphics compositing language

Pre-alpha release in progress. Priorities are basic functionalities and not clean code.

For supported nodes in this version, see [docs.md](docs.md)

Error handling is very limited.

### Pre-alpha release checklist

- [ ] Color spec methods
- [x] `io::image`
- [x] `io::output`
- [x] `spacial::crop`
- [x] `spacial::scale`
- [x] `stats::histogram`
- [x] `color::color_ramp`
- [ ] `color::mix`
- [x] `color::grayscale`

### Alpha release checklist
- [ ] Error handling
- [ ] Graph preview
- [ ] Additional node support (to be decided)

## Major project checklist

Roughly prioritized
- [ ] Pre-alpha release
- [ ] Reorganize compiler files
- [ ] Sample Eleminima code
- [ ] Clean up this file
- [ ] Alpha release
- [ ] Node graph preview image
- [ ] Release GH actions CD pipeline
- [ ] JIT
- [ ] Video demo/tutorial
- [ ] Homebrew release

## EBNF Grammar definition

```EBNF
program = { statement } ;

statement 
    = id (' ')+ definition 
    | id (' ')+ edge 
    ;

id
    = ('a' ... 'z'|'A' ... 'Z'|'_'){'0' ... '9'|'a' ... 'z'|'A' ... 'Z'|'_'}
    ;

definition 
    = '=' type '{' { field_assignment } '}'
    ;

edge
    = out_field '->' id (' ')+ in_field
    ;


out_field
    = id
    ;

in_field
    = id
    ;

type
    = category '::' node_name
    ;

category
    = 'Node'
    ;

node_name
    = 'mix'
    | 'color_ramp'
    | 'image'
    ;

field_assignment
    = in_field ':' argument [',']
    ;

argument
    = number 
    | string 
    | list 
    | fun_call
    ;

number
    = ['+'|'-'] ('0' ... '9')+ ['.' ('0' ... '9')+] /* to be improved */
    ;

string
    = '"' {/* any UTF-8 character */} '"' /* explicitly mention escaped string */
    ;

fun_call
    = id '(' {argument} ')'
    ;

list
    = '[' {argument} ']'
    ;

```

### Node
A `node` is the data type used in this language and can be one of many possible subtypes. 

The syntax to define a node is as follows:
```
Identifier = Node::*1
{
    Node::*1::opt_*... 
};

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
```elma
id_1 = Node::*1 
{
    Node::*1::opt_*
};

id_2 = Node::*2 
{
    Node::*2::opt_*
};
```
have been defined earlier.

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

1. Certain nodes may be nested, such as Adjustment nodes will be allowed to be used in multiple locations. 

## I/O

There may be multiple input or output images. 

## Nodes and options

A few representative nodes are shown below. These will be prioritized in implementation. 

```
"->" before a node means mandatory input. 
Any options can be supplied by node input

node::image
{
    src: *src_of_image*,
};
-> output image

Node::mix
{
    fac: *factor*,
};
-> output image

Node::color_ramp
{
    method: *interp_method*,
    control_points: *list_of_control_points,
};
-> output image
-> preview image

Node::output
{
    path: *path*,
};
```

## Scratch pad
```
cr = Node::color_ramp
{
    method: b-spline,
    control_points: 
    [
        [0, rgb(0, 100, 244.3)],
        [0.5, hsv(100, 50, 10)],
        [0.4, hex(FF3f00)]
    ]
};

mix = Node::mix
{
    fac: 0.618,
};

noisy = Node::image
{
    src: noisy.png,
};

denoised = Node::image
{
    src: denoised.png,
};

noisy image -> mix input_1
denoised image -> mix input_2
mix output -> output result/mixed.png
```

Required data structures:
- Lists (`[a, b, c, ...]`)
- Objects (`{ field: value }`

## AST

```
Cr = ColorRamp node with
    method: b-spline,

```

