#include <getopt.h> // arg parsing

#include "../include/adam.h"
#include "../include/cli.h"
#include "../include/support.h"


int main(int argc, char **argv)
{
  rng_data data;
  adam_init(&data);

  rng_cli cli;
  cli.data = &data;
  cli_init(&cli);

  register short opt;
  while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
    switch (opt) {
      case 'h':
        return help();
      case 'v':
      return version();
      case 'l':
        return infinite(&data);
      case 'a':
      return assess(optarg, &cli);
      case 'b':
      stream_bytes(__UINT64_MAX__, &data);
      return 0;
      case 'x':
      cli.fmt = "0x%lX";
      break;
      case 'o':
      cli.fmt = "0o%lo";
      break;      
      case 'w':
      if (set_width(&cli, optarg))
        return err("Width must be either 8, 16, 32");
      break;
      case 'r':
      // Returns all results if option is set but no argument
      // provided
      set_results(&cli, optarg);
      if (cli.results == 0)
        return err("Invalid number of results "
                   "specified for desired width");
      break;
      case 's':
      rwseed(&data.seed[0], optarg);
      break;
      case 'n':
      rwnonce(&data.nonce, optarg);
      break;
      case 'u':
      return uuid(optarg, &data);
      default:
        return err("Option is invalid or missing required argument");             
    }
  }

  adam(&data);

  print_buffer(&cli);

  return 0;
}
