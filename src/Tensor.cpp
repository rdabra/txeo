#include "txeo/Tensor.h"

#include <string>

std::string Tensor::nome_completo() {
  return nome + " " + sobrenome;
}
