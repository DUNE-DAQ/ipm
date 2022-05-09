// This is the application info schema used by the ipm receiver
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.ipm.receiverinfo");

local info = {

   count  : s.number("count", "u8", doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("bytes", self.count, 0, doc="Bytes received via the connection"),
       s.field("messages", self.count, 0, doc="Messages received via the connection"),
   ], doc="IPM receiver information")
};

moo.oschema.sort_select(info) 
