int wd = 0;
int rd = 0;
#pragma acc kernels loop independent
for(ulong i = 0; i < ULONG_MAX; i++) {
  int diff = 0;
  bool wv, rv;
  int temp_wd = read_channel_nb_altera(write_chan[buffer_id], &wv);
  int temp_rd = read_channel_nb_altera(read_chan[buffer_id], &rv);

  if(!wv && rv) {
    rd = temp_rd;
  } else if(wv && !rv) {
    wd = temp_wd;
  } else if(wv && rv) {
    wd = temp_wd;
    rd = temp_rd;
  }

  diff = wd - rd;

  ftime_t temp = get_time((ftime_t)diff);

  channel_t ch;
  ch.time = temp;
  ch.depth = diff;
  if(wv || rv)
    (void) write_channel_nb_altera(cf_chan[buffer_id], ch);
}
