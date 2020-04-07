#include "cryptol-service.h"

int main(int argc, char const *argv[]) { 
  if(argc != 3) {
    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    exit(0);
  }
  
  char ip_address[16];  
  strncpy(ip_address, argv[1], 16);
  uint32_t port = atoi(argv[2]);
  
  caas_t *caas = caas_connect(ip_address, port);
  if(caas == NULL) return 0;

  caas_load_module(caas, "Primitive::Symmetric::Cipher::Block::AES");

  bitvector_t *arg0 = bitvector_t_fromHexString("0123456789abcdef0123456789abcdef");
  json_object *jresult =
    caas_evaluate_expression(caas,
			     caas_call("msgToState",
				       caas_add_argument(NULL, caas_from_bitvector(arg0))));
  bitvector_t_free(arg0);
  json_object_put(jresult);

  jresult =
    caas_evaluate_expression(caas,
                             caas_command("msgToState 0x0123456789abcedf0123456789abcedf"));
  json_object_put(jresult);

  jresult =
    caas_evaluate_expression(caas,
                             caas_command("7 : [3]"));
  json_object_put(jresult);

  arg0 = bitvector_t_fromHexString("7f");
  bitvector_t_dropUpdate(arg0, 1);
  jresult =
    caas_evaluate_expression(caas,
			     caas_from_bitvector(arg0));
  bitvector_t_free(arg0);
  json_object_put(jresult);

  jresult =
    caas_evaluate_expression(caas,
			     caas_from_boolean(1));
  json_object_put(jresult);

  jresult =
    caas_evaluate_expression(caas,
			     caas_command("aesEncrypt(`0x1234, `0x5678)"));
  json_object_put(jresult);

  caas_disconnect(caas);
  
  return 0;
}
