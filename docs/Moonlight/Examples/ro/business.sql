-- 1. DATABASE
database create FirmaDeCurieratBD;
database use FirmaDeCurieratBD;

-- 2. SCHEMA
schema Persoana_V0 {
    nume: String;
    prenume: String;
    nr_telefon: String;
};

schema Persoana_V1 inherits Persoana_V0 {
    CNP: String;
    data_nastere: DateTime;
    email: String;
};

schema Angajat inherits Persoana_V1 {
    data_angajare: DateTime;
    salariu: Float;
    functie: String;
};

schema Client inherits Persoana_V1 {};

schema PersoanaDeContact inherits Persoana_V0 {};

schema Adresa {
    judet: String;
    oras: String;
    detalii: String;
};

schema Colet {
    curier: Angajat;
    client: Client;
    persoana_contact: PersoanaDeContact;
    adresa_livrare: Adresa;
    adresa_facturare: Adresa;
    greutate_grame: Integer;
    pret_expediere: Float;
};

schema sedintaSuport {
    operator: Angajat;
    client: Client;
    data: DateTime;
    descriere: String;
};

-- 3. CREATE

create tables from [ Colet, sedintaSuport ] using format "%TypeName%_Data";

create documents from [ Colet, sedintaSuport ] using format "%TypeName%_NOSQL";
create tables from [ Angajat, Client, Adresa ] using format "%TypeName%_SQL";

-- 4. REBIND
rebind Colet_NOSQL::curier to Angajat_SQL clean;
rebind Colet_NOSQL::client to Client_SQL clean;
rebind Colet_NOSQL::adresa_livrare to Adresa_SQL clean;
rebind Colet_NOSQL::adresa_facturare to Adresa_SQL clean;

rebind sedintaSuport_NOSQL::operator to Angajat_SQL clean;
rebind sedintaSuport_NOSQL::client to Client_SQL clean;

-- 5. MIGRATE
schema sedintaSuportExtinsa inherits sedintaSuport {
    durata: String;
};

migrate structure sedintaSuport_NOSQL to sedintaSuportExtinsa using [
    data => data_sedinta
];

-- 6. INSERT
insert into Angajat_SQL objects [
    {
        "nume": "Avramescu",
        "prenume": "Ion",
        "nr_telefon": "0729304928",
        "CNP": "5003009292012",
        "data_nastere": 22/10/1997,
        "email": "ion.avramescu@email.com",
        "data_angajare": 15/05/2020,
        "salariu": 5400.00,
        "functie": "Curier"
    }
];

-- 7. SELECT
select from Angajat_SQL
    where (lower(functie) like "Curier" and salariu between 5000 and 5500)
    fields [ nume, prenume, data_nastere, data_angajare, salariu ]
    order by [ salariu desc, data_angajare desc, data_nastere asc ];

-- 8. TRUNCATE
truncate structure Angajat_SQL;

-- 9. DROP
drop structure Angajat_SQL;

-- 10. DELETE
delete from Angajat_SQL
    where (lower(functie) like "Curier" and salariu between 5000 and 5500);

-- 11. UPDATE
update structure Angajat_SQL
    where (lower(functie) like "Curier" and salariu between 5000 and 5500)
    modify [
        salariu => salariu + salariu * 5 / 100;
    ];

-- 12. USER
user create PopescuIon;
user remove PopescuIon;

-- 13. GRANT
grant [ insert, update ] 
    to PopescuIon
    on Angajat_SQL;

-- 14. REVOKE
grant [ insert, update ] 
    from PopescuIon
    on Angajat_SQL;

-- 15. RENAME
rename structure from Angajat_SQL to Angajati_SQL;
rename field from Angajati_SQL::functie to Angajati_SQL::functie_angajat;
rename database from FirmaDeCurieratBD to FirmaDeCurierat_BD;

-- 16. COMMIT, ROLLBACK, SAVEPOINT
// tranzactii realizate...
commit;
rollback; // (1)
// alte tranzactii în derulare (2)
savepoint hash-tranzactii-set-1; // (3)
// alte tranzactii în derulare (4)
rollback hash-tranzactii-set-1; // (5)
