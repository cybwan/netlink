#include <lbrt/trie.h>

typedef struct trieVar {
  __u8 prefix[16];
} lbrt_trie_var_t;

typedef struct trieState {
  lbrt_trie_data_t trieData;
  int lastMatchLevel;
  int lastMatchPfxLen;
  bool lastMatchEmpty;
  lbrt_trie_var_t lastMatchTv;
  bool matchFound;
  int maxLevels;
  int errCode;
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

__u8 grabByte(lbrt_trie_var_t *tv, int pIndex) {
  if (pIndex > 15) {
    return 0xff;
  }
  return tv->prefix[pIndex];
}

int cidr2TrieVar(char *cidr, lbrt_trie_var_t *tv) {
  ip_net_t ipNet;
  bool success = parse_ip_net(cidr, &ipNet);
  if (!success) {
    return -1;
  }

  for (int mask = ipNet.mask, i = 0; i < 16; i++) {
    __u8 bitMask = 0xFF;
    if (mask <= (i + 1) * 8) {
      __s8 ones = mask - i * 8;
      bitMask = bitMask << (8 - ones);
      ipNet.ip.v6.bytes[i] &= bitMask;
    } else {
      ipNet.ip.v6.bytes[i] = 0;
    }
  }
  for (int i = 0; i < 16; i++) {
    tv->prefix[i] = ipNet.ip.v6.bytes[i];
  }
  return ipNet.mask;
}

void shrinkPrefixArrDat(int arr_cnt, lbrt_trie_data_t **arr, int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = startPos; i < arr_cnt - 1; i++) {
    arr[i] = arr[i + 1];
  }
}

void shrinkPtrArrDat(int arr_cnt, lbrt_trie_root_t **arr, int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = startPos; i < arr_cnt - 1; i++) {
    arr[i] = arr[i + 1];
  }
}

void expPrefixArrDat(int arr_cnt, lbrt_trie_data_t **arr, int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = arr_cnt - 2; i >= startPos; i--) {
    arr[i + 1] = arr[i];
  }
}

void expPtrArrDat(int arr_cnt, lbrt_trie_root_t **arr, int startPos) {
  if (startPos < 0 || startPos >= arr_cnt) {
    return;
  }
  for (int i = arr_cnt - 2; i >= startPos; i--) {
    arr[i + 1] = arr[i];
  }
}

// CountAllSetBitsInArr - count set bits in an array of uint8
int CountAllSetBitsInArr(int arr_cnt, __u8 *arr) {
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

// CountSetBitsInArr - count set bits in an array of uint8 from bPos
int CountSetBitsInArr(int arr_cnt, __u8 *arr, int bPos) {
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

// IsBitSetInArr - check given bPos bit is set in the array
bool IsBitSetInArr(int arr_cnt, __u8 *arr, int bPos) {
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

// SetBitInArr - set bPos bit in the array
void SetBitInArr(int arr_cnt, __u8 *arr, int bPos) {
  if (bPos >= 8 * arr_cnt) {
    return;
  }
  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);
  arr[arrIdx] |= 0x1 << bPosIdx;
}

// UnSetBitInArr - unset bPos bit in the array
void UnSetBitInArr(int arr_cnt, __u8 *arr, int bPos) {
  if (bPos >= 8 * arr_cnt) {
    return;
  }
  int arrIdx = bPos / 8;
  int bPosIdx = 7 - (bPos % 8);
  arr[arrIdx] &= ~(0x1 << bPosIdx);
}

int addTrieInt(lbrt_trie_root_t *t, lbrt_trie_var_t *tv, int currLevel,
               int rPfxLen, lbrt_trie_state_t *ts) {

  if (rPfxLen < 0 || ts->errCode != 0) {
    return -1;
  }

  // This assumes stride of length 8
  __u8 cval = tv->prefix[currLevel];
  lbrt_trie_root_t *nextRoot = NULL;

  if (rPfxLen > TrieJmpLength) {
    rPfxLen -= TrieJmpLength;
    int ptrIdx = CountSetBitsInArr(PtrArrNBits, t->ptrArr, ((int)cval) - 1);
    if (IsBitSetInArr(PtrArrNBits, t->ptrArr, (int)cval)) {
      nextRoot = t->ptrData[ptrIdx];
      if (!nextRoot) {
        ts->errCode = TrieErrUnknown;
        return -1;
      }
    } else {
      // If no pointer exists, then allocate it
      // Make pointer references
      nextRoot = calloc(1, sizeof(lbrt_trie_root_t));
      if (t->ptrData[ptrIdx]) {
        expPtrArrDat(PtrArrLength, t->ptrData, ptrIdx);
        free(t->ptrData[ptrIdx]);
        t->ptrData[ptrIdx] = NULL;
      }
      t->ptrData[ptrIdx] = nextRoot;
      SetBitInArr(PtrArrNBits, t->ptrArr, (int)cval);
    }
    return addTrieInt(nextRoot, tv, currLevel + 1, rPfxLen, ts);
  } else {
    int shftBits = TrieJmpLength - rPfxLen;
    int basePos = (1 << rPfxLen) - 1;
    // Find value relevant to currently remaining prefix len
    cval = cval >> shftBits;
    int idx = basePos + (int)cval;
    if (IsBitSetInArr(PrefixArrNbits, t->prefixArr, idx)) {
      return TrieErrExists;
    }
    int pfxIdx = CountSetBitsInArr(PrefixArrNbits, t->prefixArr, idx);
    if (t->prefixData[pfxIdx]) {
      expPrefixArrDat(PrefixArrLenfth, t->prefixData, pfxIdx);
      free(t->prefixData[pfxIdx]);
      t->prefixData[pfxIdx] = NULL;
    }
    SetBitInArr(PrefixArrNbits, t->prefixArr, idx);
    t->prefixData[pfxIdx] = ts->trieData;
    return 0;
  }
}

int deleteTrieInt(lbrt_trie_root_t *t, lbrt_trie_var_t *tv, int currLevel,
                  int rPfxLen, lbrt_trie_state_t *ts) {

  if (rPfxLen < 0 || ts->errCode != 0) {
    return -1;
  }

  // This assumes stride of length 8
  __u8 cval = tv->prefix[currLevel];
  lbrt_trie_root_t *nextRoot = NULL;

  if (rPfxLen > TrieJmpLength) {
    rPfxLen -= TrieJmpLength;
    int ptrIdx = CountSetBitsInArr(PtrArrNBits, t->ptrArr, (int)(cval - 1));
    if (!IsBitSetInArr(PtrArrNBits, t->ptrArr, (int)cval)) {
      ts->matchFound = false;
      return -1;
    }

    nextRoot = t->ptrData[ptrIdx];
    if (!nextRoot) {
      ts->matchFound = false;
      ts->errCode = TrieErrUnknown;
      return -1;
    }
    deleteTrieInt(nextRoot, tv, currLevel + 1, rPfxLen, ts);
    if (ts->matchFound && ts->lastMatchEmpty) {
      free(t->ptrData[ptrIdx]);
      t->ptrData[ptrIdx] = NULL;
      shrinkPtrArrDat(PtrArrLength, t->ptrData, ptrIdx);
      UnSetBitInArr(PtrArrNBits, t->ptrArr, (int)cval);
    }
    if (ts->lastMatchEmpty) {
      if (CountAllSetBitsInArr(PrefixArrNbits, t->prefixArr) == 0 &&
          CountAllSetBitsInArr(PtrArrNBits, t->ptrArr) == 0) {
        ts->lastMatchEmpty = true;
      } else {
        ts->lastMatchEmpty = false;
      }
    }
    if (ts->errCode != 0) {
      return -1;
    }
    return 0;
  } else {
    int shftBits = TrieJmpLength - rPfxLen;
    int basePos = (1 << rPfxLen) - 1;

    // Find value relevant to currently remaining prefix len
    cval = cval >> shftBits;
    int idx = basePos + (int)cval;
    if (!IsBitSetInArr(PrefixArrNbits, t->prefixArr, idx)) {
      ts->matchFound = false;
      return TrieErrNoEnt;
    }
    int pfxIdx = CountSetBitsInArr(PrefixArrNbits, t->prefixArr, idx - 1);
    // Note - This assumes that prefix data should be non-zero
    if (t->prefixData[pfxIdx]) {
      free(t->prefixData[pfxIdx]);
      t->prefixData[pfxIdx] = NULL;
      shrinkPrefixArrDat(PrefixArrLenfth, t->prefixData, pfxIdx);
      UnSetBitInArr(PrefixArrNbits, t->prefixArr, idx);
      ts->matchFound = true;
      if (CountAllSetBitsInArr(PrefixArrNbits, t->prefixArr) == 0 &&
          CountAllSetBitsInArr(PtrArrNBits, t->ptrArr) == 0) {
        ts->lastMatchEmpty = true;
      }
      return 0;
    }
    ts->matchFound = false;
    ts->errCode = TrieErrUnknown;
    return -1;
  }
}

int findTrieInt(lbrt_trie_root_t *t, lbrt_trie_var_t *tv, int currLevel,
                lbrt_trie_state_t *ts) {

  if (ts->errCode != 0) {
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

    if (IsBitSetInArr(PrefixArrNbits, t->prefixArr, idx)) {
      ts->lastMatchLevel = currLevel;
      ts->lastMatchPfxLen = 8 * currLevel + rPfxLen;
      ts->matchFound = true;
      ts->lastMatchTv.prefix[currLevel] = (__u8)pfxVal;
      break;
    }
  }

  cval = tv->prefix[currLevel];
  int ptrIdx = CountSetBitsInArr(PtrArrNBits, t->ptrArr, ((int)cval) - 1);
  if (IsBitSetInArr(PtrArrNBits, t->ptrArr, (int)cval)) {
    if (t->ptrData[ptrIdx]) {
      lbrt_trie_root_t *nextRoot = t->ptrData[ptrIdx];
      ts->lastMatchTv.prefix[currLevel] = (__u8)cval;
      findTrieInt(nextRoot, tv, currLevel + 1, ts);
    }
  }

  if (ts->lastMatchLevel == currLevel) {
    int pfxIdx = CountSetBitsInArr(PrefixArrNbits, t->prefixArr, idx - 1);
    ts->trieData = t->prefixData[pfxIdx];
  }

  return 0;
}

int walkTrieInt(lbrt_trie_root_t *t, lbrt_trie_var_t *tv, int level,
                lbrt_trie_state_t *ts, lbrt_trie_iter_intf_t tf) {
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
    if (IsBitSetInArr(PrefixArrNbits, t->prefixArr, p)) {
      int shftBits = TrieJmpLength - pfxLen;
      int pLevelPfxLen = level * TrieJmpLength;
      int cval = (p - basePos) << shftBits;
      if (p == 0) {
        pfxIdx = 0;
      } else {
        pfxIdx = CountSetBitsInArr(PrefixArrNbits, t->prefixArr, p - 1);
      }
      utstring_clear(pfxStr);
      utstring_clear(str);
      for (int i = 0; i < ts->maxLevels; i++) {
        int pfxVal = tv->prefix[i];
        if (i == level) {
          pfxVal = cval;
        }
        if (i == ts->maxLevels - 1) {
          utstring_printf(pfxStr, "%d", pfxVal);
        } else {
          utstring_printf(pfxStr, "%d.", pfxVal);
        }
      }
      char *td = tf.trie_data2str(t->prefixData[pfxIdx]);
      utstring_printf(str, "%20s/%d : %s", utstring_body(pfxStr),
                      pfxLen + pLevelPfxLen, td);
      tf.trie_node_walker(utstring_body(str));
    }
    n--;
  }
  for (p = 0; p < PtrArrLength; p++) {
    if (IsBitSetInArr(PtrArrNBits, t->ptrArr, p)) {
      int cval = p;
      int ptrIdx = CountSetBitsInArr(PtrArrNBits, t->ptrArr, p - 1);
      if (t->ptrData[ptrIdx]) {
        lbrt_trie_root_t *nextRoot = t->ptrData[ptrIdx];
        tv->prefix[level] = (__u8)cval;
        walkTrieInt(nextRoot, tv, level + 1, ts, tf);
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
int AddTrie(lbrt_trie_root_t *t,char *cidr, lbrt_trie_data_t *data) int {
	lbrt_trie_var_t tv;
  memset(&tv,0,sizeof(tv));
  lbrt_trie_state_t ts;
  memset(&ts,0,sizeof(ts));
  ts.maxLevels = 4;
  memcpy(&ts.trieData,data,sizeof(lbrt_trie_data_t));

	int pfxLen = cidr2TrieVar(cidr, &tv)
	if (pfxLen < 0) {
		return TrieErrPrefix;
	}

	int ret = addTrieInt(t,&tv, 0, pfxLen, &ts)
	if (ret != 0 || ts.errCode != 0) {
		return ret;
	}

	return 0;
}

void lbrt_tire_test(void) {
  lbrt_trie_var_t tv;
  memset(&tv, 0, sizeof(tv));
  cidr2TrieVar("192.168.127.0/2", &tv);
}