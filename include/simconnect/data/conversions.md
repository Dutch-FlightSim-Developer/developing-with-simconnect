# Conversions between fields and `SIMCONNECT_DATATYPE`s

## Setter

The columns represent the _source_ type, i.e., the type of the data in the incoming message, while the rows stand for the field type that value needs to be assigned to.

 type      | FLOAT32 | FLOAT64 | INT32   | INT64   | STRINGx |
-----------|---------|---------|---------|---------|---------|
 int       | cast    | cast    | direct  | cast    | throw   |
 long      | cast    | cast    | direct  | cast    | throw   |
 int32_t   | cast    | cast    | direct  | cast    | throw   |
 long long | cast    | cast    | direct  | direct  | throw   |
 int64_t   | cast    | cast    | direct  | direct  | throw   |
 float     | direct  | cast    | cast    | cast    | throw   |
 double    | direct  | direct  | direct  | cast    | throw   |
 bool      | compute | compute | compute | compute | throw   |
 string    | compute | compute | compute | compute | direct  |

## Getter

Here the columns represent how the data will be stored, with the rows for the source type of the field.

 type      | FLOAT32 | FLOAT64 | INT32   | INT64   | STRINGx |
-----------|---------|---------|---------|---------|---------|
 int       | cast    | direct  | direct  | direct  | compute |
 long      | cast    | direct  | direct  | direct  | compute |
 int32_t   | cast    | direct  | direct  | direct  | compute |
 long long | cast    | cast    | cast    | direct  | compute |
 int64_t   | cast    | cast    | cast    | direct  | compute |
 float     | direct  | direct  | cast    | cast    | compute |
 double    | cast    | direct  | cast    | cast    | compute |
 bool      | compute | compute | compute | compute | compute |
 string    | throw   | throw   | throw   | throw   | direct  |
