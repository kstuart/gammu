-- PGSQL - Update SMSD database to version 18

update gammu set "Version" = 18 where "Version" < 18;

--

alter table inbox add "MMSHeaders" text;
alter table outbox add "MMSHeaders" text;
alter table sentitems
    add "MMS_ID" varchar(126),
    add "MMSHeaders" text,
    add "MMSReports" text;

drop index if exists sentitems_mms_id;
create index sentitems_mms_id on sentitems ("MMS_ID");

--

drop table if exists inbox_mms_parts;
create table inbox_mms_parts (
    "ID" serial not null constraint inbox_mms_parts_pkey primary key,
    "INBOX_ID" integer not null constraint "inbox_mms_parts_INBOX_ID_fkey" references inbox,
    "MediaType" varchar(1024) not null,
    "Data" bytea not null
);

drop index if exists inbox_mms_parts_inbox_id;
create index inbox_mms_parts_inbox_id on inbox_mms_parts ("INBOX_ID");

--

drop table if exists outbox_mms_parts;
create table outbox_mms_parts (
    "ID" serial not null constraint outbox_mms_parts_pkey primary key,
    "OUTBOX_ID" integer not null,
    "MediaType" varchar(1024) not null,
    "DataLength" integer not null,
    "Data" bytea not null,
    "End" integer not null default 0
);

drop index if exists outbox_mms_parts_id;
create index outbox_mms_parts_id on sentitems ("ID");

drop index if exists outbox_mms_parts_outbox_id;
create index outbox_mms_parts_outbox_id on outbox_mms_parts ("OUTBOX_ID");
