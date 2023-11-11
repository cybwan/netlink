#include <lbrt/trie.h>

typedef struct lbrt_trie_var {
  __u8 prefix[16];
} lbrt_trie_var_t;

typedef struct lbrt_trie_state {
  lbrt_trie_data_t trie_data;
  int last_match_level;
  int last_match_pfx_len;
  bool last_match_empty;
  lbrt_trie_var_t last_match_tv;
  bool match_found;
  int max_levels;
  int err_code;
} lbrt_trie_state_t;

lbrt_trie_root_t *lbrt_trie_alloc(bool v6) {
  lbrt_trie_root_t *root;
  root = calloc(1, sizeof(*root));
  if (!root) {
    return NULL;
  }
  root->v6 = v6;
  return root;
}

void lbrt_trie_free(lbrt_trie_root_t *root) {
  if (!root)
    return;
  free(root);
}

__u8 __lbrt_trie_grab_byte(lbrt_trie_var_t *tv, int pIndex) {
  if (pIndex > 15) {
    return 0xff;
  }
  return tv->prefix[pIndex];
}

int __lbrt_trie_cidr_2_trie_var(char *cidr, lbrt_trie_var_t *tv) {
  ip_net_t ipNet;
  bool success = parse_ip_net(cidr, &ipNet, NULL);
  if (!success) {
    return -1;
  }

  for (int mask = ipNet.mask, i = 0; i < 16; i++) {
    __u8 bitMask = 0xFF;
    if (mask <= (i + 1) * 8) {
      __s8 ones = mask - i * 8;
      if (ones > 0) {
        bitMask = bitMask << (8 - ones);
        ipNet.ip.v6.bytes[i] &= bitMask;
      } else {
        ipNet.ip.v6.bytes[i] = 0;
      }
    }
  }
  for (int i = 0; i < 16; i++) {
    tv->prefix[i] = ipNet.ip.v6.bytes[i];
  }
  return ipNet.mask;
}

void __lbrt_trie_shrink_prefix_arr_dat(int arr_cnt, lbrt_trie_data_t *arr,
                                       int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = startPos; i < arr_cnt - 1; i++) {
    arr[i] = arr[i + 1];
  }
}

void __lbrt_trie_shrink_ptr_arr_dat(int arr_cnt, lbrt_trie_root_t **arr,
                                    int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = startPos; i < arr_cnt - 1; i++) {
    arr[i] = arr[i + 1];
  }
}

void __lbrt_trie_exp_prefix_arr_dat(int arr_cnt, lbrt_trie_data_t *arr,
                                    int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = arr_cnt - 2; i >= startPos; i--) {
    arr[i + 1] = arr[i];
  }
}

void __lbrt_trie_exp_ptr_arr_dat(int arr_cnt, lbrt_trie_root_t **arr,
                                 int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = arr_cnt - 2; i >= startPos; i--) {
    arr[i + 1] = arr[i];
  }
}

// CountAllSetBitsInArr - count set bits in an array of uint8
int __lbrt_trie_count_all_set_bits_in_arr(int arr_cnt, __u8 *arr) {
  int bCount = 0;
  for (int i = 0; i < arr_cnt; i++) {
    for (int n = 0; n < 8; n++) {
      if ((arr[i] >> n) & 0x1) {
        bCount++;
      }
    }
  }
  return bCount;
}

// __lbrt_trie_count_set_bits_in_arr - count set bits in an array of uint8 from
// bPos
int __lbrt_trie_count_set_bits_in_arr(int arr_cnt, __u8 *arr, int bPos) {
  int bCount = 0;
  if (bPos >= 8 * arr_cnt) {
    return -1;
  }

  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);

  for (int i = 0; i <= arrIdx; i++) {
    __u8 val;
    if (i == arrIdx) {
      val = arr[i] >> bPosIdx & 0xff;
    } else {
      val = arr[i];
    }
    for (int n = 0; n < 8; n++) {
      if ((val >> n) & 0x1) {
        bCount++;
      }
    }
  }
  return bCount;
}

// __lbrt_trie_is_bit_set_in_arr - check given bPos bit is set in the array
bool __lbrt_trie_is_bit_set_in_arr(int arr_cnt, __u8 *arr, int bPos) {
  if (bPos >= 8 * arr_cnt) {
    return false;
  }
  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);
  if (((arr[arrIdx] >> bPosIdx) & 0x1) == 0x1) {
    return true;
  }
  return false;
}

// __lbrt_trie_set_bit_in_arr - set bPos bit in the array
void __lbrt_trie_set_bit_in_arr(int arr_cnt, __u8 *arr, int bPos) {
  if (bPos >= 8 * arr_cnt) {
    return;
  }
  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);
  arr[arrIdx] |= 0x1 << bPosIdx;
}

// __lbrt_trie_unset_bit_in_arr - unset bPos bit in the array
void __lbrt_trie_unset_bit_in_arr(int arr_cnt, __u8 *arr, int bPos) {
  if (bPos >= 8 * arr_cnt) {
    return;
  }
  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);
  arr[arrIdx] &= ~(0x1 << bPosIdx);
}

int __lbrt_trie_add_trie_int(lbrt_trie_root_t *t, lbrt_trie_var_t *tv,
                             int currLevel, int rPfxLen,
                             lbrt_trie_state_t *ts) {
  if (rPfxLen < 0 || ts->err_code != 0) {
    return -1;
  }

  // This assumes stride of length 8
  __u8 cval = tv->prefix[currLevel];
  lbrt_trie_root_t *nextRoot = NULL;

  if (rPfxLen > TrieJmpLength) {
    rPfxLen -= TrieJmpLength;
    int ptrIdx = __lbrt_trie_count_set_bits_in_arr(PtrArrNBits, t->ptrArr,
                                                   ((int)cval) - 1);
    if (__lbrt_trie_is_bit_set_in_arr(PtrArrNBits, t->ptrArr, (int)cval)) {
      nextRoot = t->ptrData[ptrIdx];
      if (!nextRoot) {
        ts->err_code = TRIE_ERR_UNKNOWN;
        return -1;
      }
    } else {
      // If no pointer exists, then allocate it
      // Make pointer references
      nextRoot = calloc(1, sizeof(lbrt_trie_root_t));
      nextRoot->v6 = t->v6;
      if (t->ptrData[ptrIdx]) {
        __lbrt_trie_exp_ptr_arr_dat(PtrArrLength, t->ptrData, ptrIdx);
        t->ptrData[ptrIdx] = NULL;
      }
      t->ptrData[ptrIdx] = nextRoot;
      __lbrt_trie_set_bit_in_arr(PtrArrNBits, t->ptrArr, (int)cval);
    }
    return __lbrt_trie_add_trie_int(nextRoot, tv, currLevel + 1, rPfxLen, ts);
  } else {
    int shftBits = TrieJmpLength - rPfxLen;
    int basePos = (1 << rPfxLen) - 1;
    // Find value relevant to currently remaining prefix len
    cval = cval >> shftBits;
    int idx = basePos + (int)cval;
    if (__lbrt_trie_is_bit_set_in_arr(PrefixArrNbits, t->prefixArr, idx)) {
      return TRIE_ERR_EXISTS;
    }
    int pfxIdx =
        __lbrt_trie_count_set_bits_in_arr(PrefixArrNbits, t->prefixArr, idx);
    if (t->prefixData[pfxIdx].f.osid || t->prefixData[pfxIdx].f.neigh) {
      __lbrt_trie_exp_prefix_arr_dat(PrefixArrLenfth, t->prefixData, pfxIdx);
      memset(&t->prefixData[pfxIdx], 0, sizeof(lbrt_trie_data_t));
    }
    __lbrt_trie_set_bit_in_arr(PrefixArrNbits, t->prefixArr, idx);
    memcpy(&t->prefixData[pfxIdx], &ts->trie_data, sizeof(lbrt_trie_data_t));
    return 0;
  }
}

int __lbrt_trie_del_trie_int(lbrt_trie_root_t *t, lbrt_trie_var_t *tv,
                             int currLevel, int rPfxLen,
                             lbrt_trie_state_t *ts) {

  if (rPfxLen < 0 || ts->err_code != 0) {
    return -1;
  }

  // This assumes stride of length 8
  __u8 cval = tv->prefix[currLevel];
  lbrt_trie_root_t *nextRoot = NULL;

  if (rPfxLen > TrieJmpLength) {
    rPfxLen -= TrieJmpLength;
    int ptrIdx = __lbrt_trie_count_set_bits_in_arr(PtrArrNBits, t->ptrArr,
                                                   (int)(cval - 1));
    if (!__lbrt_trie_is_bit_set_in_arr(PtrArrNBits, t->ptrArr, (int)cval)) {
      ts->match_found = false;
      return -1;
    }

    nextRoot = t->ptrData[ptrIdx];
    if (!nextRoot) {
      ts->match_found = false;
      ts->err_code = TRIE_ERR_UNKNOWN;
      return -1;
    }
    __lbrt_trie_del_trie_int(nextRoot, tv, currLevel + 1, rPfxLen, ts);
    if (ts->match_found && ts->last_match_empty) {
      free(t->ptrData[ptrIdx]);
      t->ptrData[ptrIdx] = NULL;
      __lbrt_trie_shrink_ptr_arr_dat(PtrArrLength, t->ptrData, ptrIdx);
      __lbrt_trie_unset_bit_in_arr(PtrArrNBits, t->ptrArr, (int)cval);
    }
    if (ts->last_match_empty) {
      if (__lbrt_trie_count_all_set_bits_in_arr(PrefixArrNbits, t->prefixArr) ==
              0 &&
          __lbrt_trie_count_all_set_bits_in_arr(PtrArrNBits, t->ptrArr) == 0) {
        ts->last_match_empty = true;
      } else {
        ts->last_match_empty = false;
      }
    }
    if (ts->err_code != 0) {
      return -1;
    }
    return 0;
  } else {
    int shftBits = TrieJmpLength - rPfxLen;
    int basePos = (1 << rPfxLen) - 1;

    // Find value relevant to currently remaining prefix len
    cval = cval >> shftBits;
    int idx = basePos + (int)cval;
    if (!__lbrt_trie_is_bit_set_in_arr(PrefixArrNbits, t->prefixArr, idx)) {
      ts->match_found = false;
      return TRIE_ERR_NO_ENT;
    }
    int pfxIdx = __lbrt_trie_count_set_bits_in_arr(PrefixArrNbits, t->prefixArr,
                                                   idx - 1);
    // Note - This assumes that prefix data should be non-zero
    if (t->prefixData[pfxIdx].f.osid || t->prefixData[pfxIdx].f.neigh) {
      memset(&t->prefixData[pfxIdx], 0, sizeof(lbrt_trie_data_t));
      __lbrt_trie_shrink_prefix_arr_dat(PrefixArrLenfth, t->prefixData, pfxIdx);
      __lbrt_trie_unset_bit_in_arr(PrefixArrNbits, t->prefixArr, idx);
      ts->match_found = true;
      if (__lbrt_trie_count_all_set_bits_in_arr(PrefixArrNbits, t->prefixArr) ==
              0 &&
          __lbrt_trie_count_all_set_bits_in_arr(PtrArrNBits, t->ptrArr) == 0) {
        ts->last_match_empty = true;
      }
      return 0;
    }
    ts->match_found = false;
    ts->err_code = TRIE_ERR_UNKNOWN;
    return -1;
  }
}

int __lbrt_trie_find_trie_int(lbrt_trie_root_t *t, lbrt_trie_var_t *tv,
                              int currLevel, lbrt_trie_state_t *ts) {

  if (ts->err_code != 0) {
    return -1;
  }

  int idx = 0;
  // This assumes stride of length 8
  __u8 cval = tv->prefix[currLevel];

  for (int rPfxLen = TrieJmpLength; rPfxLen >= 0; rPfxLen--) {
    int shftBits = TrieJmpLength - rPfxLen;
    int basePos = (1 << rPfxLen) - 1;
    // Find value relevant to currently remaining prefix len
    cval = cval >> shftBits;
    idx = basePos + (int)cval;
    int pfxVal = (idx - basePos) << shftBits;
    if (__lbrt_trie_is_bit_set_in_arr(PrefixArrNbits, t->prefixArr, idx)) {
      ts->last_match_level = currLevel;
      ts->last_match_pfx_len = 8 * currLevel + rPfxLen;
      ts->match_found = true;
      ts->last_match_tv.prefix[currLevel] = (__u8)pfxVal;
      break;
    }
  }

  cval = tv->prefix[currLevel];
  int ptrIdx = __lbrt_trie_count_set_bits_in_arr(PtrArrNBits, t->ptrArr,
                                                 ((int)cval) - 1);
  if (__lbrt_trie_is_bit_set_in_arr(PtrArrNBits, t->ptrArr, (int)cval)) {
    if (t->ptrData[ptrIdx]) {
      lbrt_trie_root_t *nextRoot = t->ptrData[ptrIdx];
      ts->last_match_tv.prefix[currLevel] = (__u8)cval;
      __lbrt_trie_find_trie_int(nextRoot, tv, currLevel + 1, ts);
    }
  }

  if (ts->last_match_level == currLevel) {
    int pfxIdx = __lbrt_trie_count_set_bits_in_arr(PrefixArrNbits, t->prefixArr,
                                                   idx - 1);
    ts->trie_data = t->prefixData[pfxIdx];
  }

  return 0;
}

int __lbrt_trie_walk_trie_int(lbrt_trie_root_t *t, lbrt_trie_var_t *tv,
                              int level, lbrt_trie_state_t *ts,
                              lbrt_trie_iter_intf_t tf) {
  int p;
  int pfxIdx;
  UT_string *pfxStr, *str;

  int n = 1;
  int pfxLen = 0;
  int basePos = 0;

  utstring_new(pfxStr);
  utstring_new(str);

  for (p = 0; p < PrefixArrLenfth; p++) {
    if (n <= 0) {
      pfxLen++;
      n = 1 << pfxLen;
      basePos = n - 1;
    }
    if (__lbrt_trie_is_bit_set_in_arr(PrefixArrNbits, t->prefixArr, p)) {
      int shftBits = TrieJmpLength - pfxLen;
      int pLevelPfxLen = level * TrieJmpLength;
      int cval = (p - basePos) << shftBits;
      if (p == 0) {
        pfxIdx = 0;
      } else {
        pfxIdx = __lbrt_trie_count_set_bits_in_arr(PrefixArrNbits, t->prefixArr,
                                                   p - 1);
      }
      utstring_clear(pfxStr);
      utstring_clear(str);
      for (int i = 0; i < ts->max_levels; i++) {
        int pfxVal = tv->prefix[i];
        if (i == level) {
          pfxVal = cval;
        }
        if (i == ts->max_levels - 1) {
          utstring_printf(pfxStr, "%d", pfxVal);
        } else {
          utstring_printf(pfxStr, "%d.", pfxVal);
        }
      }
      char td[16];
      memset(td, 0, 16);
      tf.trie_data2str(&t->prefixData[pfxIdx], 16, td);
      utstring_printf(str, "%20s/%d : %s", utstring_body(pfxStr),
                      pfxLen + pLevelPfxLen, td);
      tf.trie_node_walker(utstring_body(str));
    }
    n--;
  }
  for (p = 0; p < PtrArrLength; p++) {
    if (__lbrt_trie_is_bit_set_in_arr(PtrArrNBits, t->ptrArr, p)) {
      int cval = p;
      int ptrIdx =
          __lbrt_trie_count_set_bits_in_arr(PtrArrNBits, t->ptrArr, p - 1);
      if (t->ptrData[ptrIdx]) {
        lbrt_trie_root_t *nextRoot = t->ptrData[ptrIdx];
        tv->prefix[level] = (__u8)cval;
        __lbrt_trie_walk_trie_int(nextRoot, tv, level + 1, ts, tf);
      }
    }
  }
  utstring_free(pfxStr);
  utstring_free(str);
  return 0;
}

// AddTrie - Add a trie entry
// cidr is the route in cidr format and data is any user-defined data
// returns 0 on success or non-zero error code on error
int lbrt_trie_add(lbrt_trie_root_t *t, char *cidr, lbrt_trie_data_t *data) {
  lbrt_trie_var_t tv;
  memset(&tv, 0, sizeof(tv));
  lbrt_trie_state_t ts;
  memset(&ts, 0, sizeof(ts));
  ts.max_levels = 4;
  memcpy(&ts.trie_data, data, sizeof(lbrt_trie_data_t));

  int pfxLen = __lbrt_trie_cidr_2_trie_var(cidr, &tv);
  if (pfxLen < 0) {
    return TRIE_ERR_PREFIX;
  }

  int ret = __lbrt_trie_add_trie_int(t, &tv, 0, pfxLen, &ts);
  if (ret != 0 || ts.err_code != 0) {
    return ret;
  }

  return 0;
}

// DelTrie - Delete a trie entry
// cidr is the route in cidr format
// returns 0 on success or non-zero error code on error
int lbrt_trie_del(lbrt_trie_root_t *t, char *cidr) {
  lbrt_trie_var_t tv;
  memset(&tv, 0, sizeof(tv));
  lbrt_trie_state_t ts;
  memset(&ts, 0, sizeof(ts));
  ts.max_levels = 4;

  int pfxLen = __lbrt_trie_cidr_2_trie_var(cidr, &tv);
  if (pfxLen < 0) {
    return TRIE_ERR_PREFIX;
  }

  int ret = __lbrt_trie_del_trie_int(t, &tv, 0, pfxLen, &ts);
  if (ret != 0 || ts.err_code != 0) {
    return ret;
  }

  return 0;
}

// FindTrie - Lookup matching route as per longest prefix match
// IP is the IP address in string format
// returns the following :
// 1. 0 on success or non-zero error code on error
// 2. matching route in *net.IPNet form
// 3. user-defined data associated with the trie entry
int lbrt_trie_find(lbrt_trie_root_t *t, char *ip, ip_net_t *ipnet,
                   lbrt_trie_data_t *trie_data) {
  lbrt_trie_var_t tv;
  memset(&tv, 0, sizeof(tv));
  lbrt_trie_state_t ts;
  memset(&ts, 0, sizeof(ts));
  ts.max_levels = 4;

  char cidr[IF_CIDRSIZE];
  memset(cidr, 0, IF_CIDRSIZE);

  memset(ipnet, 0, sizeof(ip_net_t));
  memset(trie_data, 0, sizeof(lbrt_trie_data_t));

  if (!t->v6) {
    snprintf(cidr, IF_CIDRSIZE, "%s/32", ip);
  } else {
    snprintf(cidr, IF_CIDRSIZE, "%s/128", ip);
  }
  int pfxLen = __lbrt_trie_cidr_2_trie_var(cidr, &tv);

  if (pfxLen < 0) {
    return TRIE_ERR_PREFIX;
  }

  __lbrt_trie_find_trie_int(t, &tv, 0, &ts);

  if (ts.match_found) {
    if (!t->v6) {
      ipnet->mask = ts.last_match_pfx_len;
      ipnet->ip.f.v4 = 1;
      memcpy(ipnet->ip.v4.bytes, ts.last_match_tv.prefix, 4);
      memcpy(trie_data, &ts.trie_data, sizeof(lbrt_trie_data_t));
      return 0;
    } else {
      ipnet->mask = ts.last_match_pfx_len;
      ipnet->ip.f.v6 = 1;
      memcpy(ipnet->ip.v6.bytes, ts.last_match_tv.prefix, 16);
      memcpy(trie_data, &ts.trie_data, sizeof(lbrt_trie_data_t));
      return 0;
    }
  }
  return TRIE_ERR_NO_ENT;
}

// Trie2String - stringify the trie table
void lbrt_trie_str(lbrt_trie_root_t *t, lbrt_trie_iter_intf_t tf) {
  lbrt_trie_var_t tv;
  memset(&tv, 0, sizeof(tv));
  lbrt_trie_state_t ts;
  memset(&ts, 0, sizeof(ts));
  ts.max_levels = 4;
  __lbrt_trie_walk_trie_int(t, &tv, 0, &ts, tf);
}