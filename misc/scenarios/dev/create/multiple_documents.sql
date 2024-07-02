database drop test_db;
database create test_db;
database use test_db;
schema schema01 { field1: String; field2: Integer; };
schema schema02 { str: String; obj: schema01; };

create collections from [ schema01, schema02 ] using format "%TypeName%_Document";

insert into schema02_Document objects [  
  { "str": "val11", "obj": { "field1": "val11", "field2": "11" } }  
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val99", "obj": { "field1": "val99", "field2": "99" } }  
];
select from structure schema02_Document where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
