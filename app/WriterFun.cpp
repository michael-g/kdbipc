#include "KdbType.h"
#include <cstdlib>
#include <string_view>


namespace mg7x {

template<typename...Args>
WriteResult write(WriteBuf & buf, Args ... args)
{
  WriteResult rtn = WriteResult::WR_OK;
  // From https://stackoverflow.com/a/60136761/322304
  ([&]{
    if (WriteResult::WR_OK == rtn) {
      rtn = args.write(buf);
    }
  }(),...);
  return rtn;
}

};

using namespace mg7x;

int main(int argc, char *argv[])
{
  uint8_t mem[1024];
  WriteBuf buf{mem, sizeof mem};
  write<KdbSymbolAtom, KdbSymbolAtom, KdbSymbolAtom, KdbIntAtom, KdbFloatAtom>(buf, std::string_view{".u.upd"}, std::string_view{"trade"}, std::string_view{"VOD.L"}, {42}, {42.});
  return EXIT_SUCCESS;
}