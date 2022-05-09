// This is the application info schema used by the ipm sender
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.ipm.senderinfo");

local info = {

   count  : s.number("count", "u8", doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("bytes", self.count, 0, doc="Bytes sent via the connection"),
       s.field("messages", self.count, 0, doc="Messages sent via the connection"),
   ], doc="IPM sender information")
};

moo.oschema.sort_select(info) 
