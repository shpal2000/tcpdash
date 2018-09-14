#include <stdio.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

int main(int argc, char **argv) {
  JsonNode *root;
  GError *error;

  static const gchar *foo_object_json = "{\"bar\"  : 42,\"baz\"  : true,\"blah\" : \"bravo\"}";

  root = json_from_string (foo_object_json, &error);
  error = (GError*)root;
}