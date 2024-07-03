database drop SomeDatabase;
database create SomeDatabase;
database use SomeDatabase;
database backup SomeDatabase to disk "/home/user/lunardb-backup";

user create SomeUser password SomePassword;
auth SomeUser;
grant [ select,  update, insert, delete ] to UserName on SomeCollection;
revoke [ select,  update, insert, delete ] from UserName on SomeCollection;

commit;
savepoint [hash];
rollback [hash];

schema Dummy {
    field01 : String;
    field02 : Integer;
}

schema SomeSchema01 {
    field_1: String;
    field_2: Boolean;
    field_3: DateTime;
    field_4: Integer;
    field_5: Float;
    field_6: Dummy;
};

create table|collection SomeCollection based on SomeSchema01;

truncate structure SomeStructure;
drop structure SomeStructure;


insert into SomeStructure objects [                               
    {                                                             
         "salary": "4000",                                        
         "name": "Bob",                                           
         "birth_date": "09/10/1985",                              
         "address": {                                             
             "street": "Some Street",                             
             "number": "5"                                        
         }                                                        
    }                                                             
    {                                                             
         "salary": "4000",                                        
         "name": "Bob",                                           
         "birth_date": "09/10/1985",                              
         "address": {                                             
             "street": "Some Street",                             
             "number": "5"                                        
         }                                                        
    }                                                             
    ...                                                        
];

select from structure StructureName where ( WHERE_CLAUSE ) fields [ field1, field2, field3 ];

update structure StructureName where ( WHERE_CLAUSE );
delete from structure StructureName where ( WHERE_CLAUSE );

update structure StructureName                    
     where (                                      
         name like "*escu"                        
         and salary between 5500 and 6000         
         or (                                     
                (                                
                   salary < 5000                  
                   or salary >= 5200              
                )                                 
                or salary > 6500                  
                and profession in [Prof1, Prof2]  
             )                                    
      )                                           
     modify [                                     
         field1 => field1 * 1.5 + 2,              
         field2 => field3 + "_some_suffix"        
     ];
