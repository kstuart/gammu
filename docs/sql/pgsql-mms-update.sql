drop table if exists inbox_mms_parts;

create table inbox_mms_parts
(
    "ID" serial not null primary key,
    "INBOX_ID" integer not null references inbox("ID"),
    "MediaType" varchar(255),
    "Data" bytea
);

drop table if exists outbox_mms_parts;

create table outbox_mms_parts
(
    "ID" serial not null primary key,
    "OUTBOX_ID" integer,
    "MediaType" varchar(255),
    "DataLength" integer not null,
    "Data" bytea,
    "End" integer
);

-- For Sent MMS, UDH is used to store MMS MessageID for report matching
create unique index sentitems_UDH_uindex on sentitems ("UDH");