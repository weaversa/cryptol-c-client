#include "cryptol-service.h"

void rand_str(char *str, uint32_t nLength) {
  uint32_t i;
  for(i = 0; i < nLength; i++) {
    sprintf(str + i, "%x", rand() % 16);
  }
}

void bvTest(caas_t *caas) {
  char str[32];

  //1st test
  rand_str(str, 32);
  bitvector_t *a = bitvector_t_fromHexString(str);

  bitvector_t_widenUpdate(a, rand()%8);
  
  json_object *aj = caas_from_bitvector_t(a);

  bitvector_t *b = caas_bitvector_t_from_bits(aj);
  json_object_put(aj);

  uint8_t equal = bitvector_t_equal(a, b);
  fprintf(stdout, "a==b ? %u\n", equal);

  assert(equal == 1);

  bitvector_t_free(a);
  bitvector_t_free(b);

  //2nd test  
  uint32_t nLength = rand()%10;
  sequence_t *seq = sequence_t_alloc(0);

  for(;nLength > 0; nLength--) {
    rand_str(str, 32);
    sequence_t_push(seq, *bitvector_t_fromHexString(str));
  }

  json_object *jresult =
    caas_evaluate_expression(caas,
			     caas_command(sequence_t_toCryptolString(seq)));
  json_object *jvalue = caas_get_value(jresult);

  sequence_t *seq_val = caas_sequence_t_from_sequence(jvalue);
  
  json_object_put(jresult);

  if(seq_val != NULL) {
    uint32_t i;
    for(i = 0; i < seq->nLength; i++) {
      equal = bitvector_t_equal(&seq->pList[i], &seq_val->pList[i]);
      fprintf(stdout, "a%u==b%u ? %u\n", i, i, equal);
      assert(equal == 1);
    }
    sequence_t_free(seq_val, bitvector_t_free_inner);
  }

  sequence_t_free(seq, bitvector_t_free_inner);
}

void AESTest(caas_t *caas) {
  //Load AES
  caas_load_module(caas, "Primitive::Symmetric::Cipher::Block::AES");

  //1st test
  char str[32];

  rand_str(str, 32);
  bitvector_t *key = bitvector_t_fromHexString(str);

  rand_str(str, 32);
  bitvector_t *pt = bitvector_t_fromHexString(str);
  
  json_object *jresult =
    caas_evaluate_expression(caas,
			     caas_call("aesEncrypt", caas_add_argument(NULL, 
				       caas_add_to_tuple(
				       caas_add_to_tuple(NULL, caas_from_bitvector_t(pt)),
                                                               caas_from_bitvector_t(key))
				      ))
			     );
  json_object_put(jresult);

  //2nd test
  char command[300];
  snprintf(command, 256+15, "aesEncrypt(%s, %s)", bitvector_t_toCryptolString(pt), bitvector_t_toCryptolString(key));
  jresult = caas_evaluate_expression(caas, caas_command(command));
  json_object_put(jresult);

  bitvector_t_free(pt);
  bitvector_t_free(key);
}

void p512Test(caas_t *caas) {
  //Load p512
  caas_load_module(caas, "Primitive::Asymmetric::Signature::ECDSA::p521");

  char str[130];
  
  rand_str(str, 130);
  bitvector_t *d = bitvector_t_fromHexString(str);

  rand_str(str, 130);
  bitvector_t *msgHash = bitvector_t_fromHexString(str);

  rand_str(str, 130);
  bitvector_t *k = bitvector_t_fromHexString(str);
  
  json_object *jresult =
  caas_evaluate_expression(caas,
			     caas_call("sign",  caas_add_argument(caas_add_argument(caas_add_argument(NULL,
			       caas_call("BVtoZ", caas_add_argument(NULL, caas_from_bitvector_t(d)))),
			       caas_call("BVtoZ", caas_add_argument(NULL, caas_from_bitvector_t(msgHash)))),
  			       caas_call("BVtoZ", caas_add_argument(NULL, caas_from_bitvector_t(k))))
			     )
			   );
  json_object_put(jresult);  
}

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

  //Load AES
  caas_load_module(caas, "Primitive::Symmetric::Cipher::Block::AES");
  
  bitvector_t *arg0 = bitvector_t_fromHexString("0123456789abcdef0123456789abcdef");
  json_object *jresult =
    caas_evaluate_expression(caas,
			     caas_call("msgToState",
				       caas_add_argument(NULL, caas_from_bitvector_t(arg0))));
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
			     caas_from_bitvector_t(arg0));
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

  while(1) {
    AESTest(caas);
    bvTest(caas);
    //p512Test(caas);
  }
  
  caas_disconnect(caas);
  
  return 0;
}
