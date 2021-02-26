// The schema used by classes in the ipm code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/plugins/VectorIntIPM* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.ipm.vectorintipmsenderdaqmodule";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local types = {
    size_t_attempt: s.number("Size_t", "u8",
                     doc="Same as a size_t in gcc v8.2.0"),

    int_attempt: s.number("Int", "i4",
                     doc="Same as an int in gcc v8.2.0"),
    string_attempt: s.string("String", "string", doc="String in gcc v8.2.0"),
    conninfo: s.any("ConnectionInfo", doc="Connection Info passed to connect_for_receives"),

    conf: s.record("Conf", [
        s.field("nIntsPerVector", self.size_t_attempt, 10,
                doc="Number of numbers"),
        s.field("queue_timeout_ms", self.int_attempt, 100,
                doc="Milliseconds to wait on queue before timing out"),
        s.field("topic", self.string_attempt, "", doc="Optional metadata to include in sends"),
        s.field("sender_type", self.string_attempt, "", doc="IPMSender Implementation Plugin to load"),
        s.field("connection_info", self.conninfo, doc="Conneection Info"),
    ], doc="VectorIntIPMSenderDAQModule Configuration"),

};

moo.oschema.sort_select(types, ns)

