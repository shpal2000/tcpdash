#include <stdio.h>
#include <inttypes.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

int main(int argc, char **argv) {
  JsonNode *root;
  GError *error;

  static const gchar *foo_object_json = "{\"bar\"  : 42,\"baz\"  : true,\"blah\" : \"bravo\", \"arr\" : [20, 30, 40] }";

  root = json_from_string (foo_object_json, &error);

  JsonObject* rootObject = json_node_get_object (root);
  JsonNode* barNode = json_object_get_member (rootObject, "bar");
  JsonNode* bazNode = json_object_get_member (rootObject, "baz");
  JsonNode* blahNode = json_object_get_member (rootObject, "blah");
  JsonNode* arrNode = json_object_get_member (rootObject, "arr");

  uint64_t barInt = json_node_get_int (barNode); 
  int bazBool = json_node_get_boolean (bazNode);
  const char* blahString = json_node_get_string (blahNode);
  JsonArray *	jsonArr = json_node_get_array (arrNode);
  uint64_t arrList[10];
  int jsonArrLength = json_array_get_length(jsonArr);

  int arrCount = 0;
  for (int i =0; i < 10 && i < jsonArrLength; i++) {
    arrList[i] = json_array_get_int_element(jsonArr, i);
    arrCount++;
  }

  for (int i =0; i < arrCount; i++) {
    printf ("%" PRIu64 "\n", arrList[i]);
  }

  printf("bar = %" PRIu64 ", baz=%i, blah = %s\n", barInt, bazBool, blahString);

  return 0;
}