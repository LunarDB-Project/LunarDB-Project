database drop Scenario02_DB;
database create Scenario02_DB;
database use Scenario02_DB;
schema Scenario02_Schema { name: string; salary: float; };
create collection Scenario02_Collection based on Scenario02_Schema;
insert into Scenario02_Collection objects [ {"name":"Bob","salary":"4000"} {"name":"George","salary":"4500"} {"name":"Akshan","salary":"1000000"} ];
select from structure Scenario02_Collection where ( 1 == 1 ) fields [ _rid, salary, name ];