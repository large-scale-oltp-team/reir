RErational Intermediate Representation(REIR)'s language specification.
=======================================================================

## Table Definition

Define table with name and array of columns information.

```
define<type_name:column_name[, <type_name:column_name>]> table_name
```

Parameters
----------

- table name: arbitrary strings excepts reserved keywords.
- typename:
  - int: 64-bit signed integer
  - string: arbitrary length variable length character
  - float: 64-bit floating point.
- options:
  - key: annotate this column as key

Known Issue
------

- At least 1 column must be key. if no key specified, it *does not* emit error and mulfunction.
- Only int type is implemented.


Table Definition Example
------------------------

creates table `hoge` with two columns, first column named `a` is key and integer typed, second column named `b` is not key and integer typed.
```
define<int:foo, int:bar> mytable
```

## Table Truncation

Drop table with specified name.

```
drop_table(<name>)
```

Table Truncation Example
------------------------

Dropping a table named `hoge`.

```
drop_table(hoge)
```

## Table Insertion

Insert a row into specified table.

```
insert(<table name>, <value>...)
```

----------------------------Not implemented border---------------------------------------

# Types

## Integer

64-bit integer number.
Integer literal is simply like `1` `2` `3`.

## Strings

Variable length string.
String literal is double quated like `"some string"`

## Float

64-bit precision floating point number.
FP literal is like `12.23` `12.34f` both are the same value.

## RowLiteral

Arbitrary tuple of rows.
RowLiteral literal is written with braces like `{2 "foo" 3.4f}`.
You can access its member with number.
`{2 "foo" 3.4f}[0]` is `2`.

## ArrayLiteral

Arbitrary array of values.
All entry must have the same type.
ArrayLiteral literal is written with brackets like `[1 2 3 4]`.
You can use it as array of row like `[{1 "a"} {2 "b"}]`.
But row is typed, so that `[{1 "a"} {2 3}]` is not allowed (second element's type is not the same with first element's).
Empty array is `[]`.

# Variables

## Assign

```
let <variable name> = <value>
```

#### Assignment Example

```
let a = 10 # assign 10 into variable named `a`
let arr = [1 2 3 4]  # assign 4 cells row into arr
```

#### In Plan(Not implemented yet)

It should be able to insert mutlitple rows with one function

```
insert(<table name>, [<value>...])
```

# Calculation

## Arithmetic calculation

```
1 + 2 + 3  # 6
8 - 1 - 2  # 5
2 * 3 * 7  # 42
15 / 5 / 2 # 1
15.0 / 5 / 2 # 1.5(not supported yet)
```

# Control

## Transaction

```
transaction <isolation level> {
  ...
}
```


## Condition

```
if <condition> {
  <true sequence>
} [else {
  <false sequence>
}]
```


## Print message (for debug)

```
(print <variable name>)
```

## output result

```
emit(<row>)
```

### output result example

Return query result as value

```
(emit [1 2 3])
```

Emitted value's types are statically checked.

## Table Scan

Full scan of table, it costs propotional to number of rows.
You must implement function for each rows
Full-scan requires sequence of instructions.

```
(full\_scan <table name>
    (<colmun name> <column name>...)
    ...
)
```

### Table scan example

```
# SELECT b,c FROM foo;
define<{int:x key, int:y, int:z}> foo
transaction {
  scan foo, row {
	emit row
  }
}
```

```
# SELECT * FROM foo where b == 1;
define<{int:a key, int:b, int:c}> foo
transaction {
  scan foo, row {
    if row.b == 1 {
      emit row
    }
  }
}

```

```
# SELECT * FROM foo,bar where foo.a == bar.x;
# Nested loop join
(full\_scan foo
    (a b c)
    (full\_scan bar
        (x y)
        (if (= a x)
            (emit [a b c x y])
        ()  # do nothing
    )))
```

