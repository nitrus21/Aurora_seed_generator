"""Apply AURORA's RAM-clearing hardening to the pinned uBitcoin sources.

PlatformIO downloads libraries outside the project source tree.  Keeping the
patch here makes the hardening reproducible after a clean dependency install.
Every replacement is fail-closed: a changed upstream source stops the build
instead of silently compiling without the expected memory clearing.
"""

from pathlib import Path

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.


MARKER = "AURORA_UBITCOIN_RAM_HARDENING_V1"


def replace_once(text, old, new, source):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            "AURORA: unexpected uBitcoin source in %s (pattern count %d)"
            % (source, count)
        )
    return text.replace(old, new, 1)


def patch_file(path, replacements):
    if not path.is_file():
        raise RuntimeError("AURORA: missing pinned uBitcoin file: %s" % path)
    text = path.read_text(encoding="utf-8")
    if MARKER in text:
        for _, expected in replacements:
            if text.count(expected) != 1:
                raise RuntimeError(
                    "AURORA: incomplete uBitcoin hardening in %s" % path.name
                )
        return False
    for old, new in replacements:
        text = replace_once(text, old, new, path.name)
    text = "// %s\n%s" % (MARKER, text)
    path.write_text(text, encoding="utf-8", newline="\n")
    return True


lib_root = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env.subst("$PIOENV")
    / "uBitcoin"
    / "src"
)

changed = False

changed |= patch_file(
    lib_root / "Hash.h",
    [
        (
            '#include "utility/trezor/hmac.h"\n',
            '#include "utility/trezor/hmac.h"\n'
            '#include "utility/trezor/memzero.h"\n',
        ),
        (
            "    RMD160(){ begin(); };\n",
            "    RMD160(){ begin(); };\n"
            "    ~RMD160(){ memzero(&ctx, sizeof(ctx)); };\n",
        ),
        (
            "    SHA256(){ begin(); };\n",
            "    SHA256(){ begin(); };\n"
            "    virtual ~SHA256(){ memzero(&ctx, sizeof(ctx)); };\n",
        ),
        (
            "    SHA512(){ begin(); };\n",
            "    SHA512(){ begin(); };\n"
            "    ~SHA512(){ memzero(&ctx, sizeof(ctx)); };\n",
        ),
    ],
)

# uBitcoin's TaggedHash constructor already feeds SHA256(tag) twice into the
# context.  Calling hashData() afterwards used to call begin() a second time,
# discarding that BIP340/BIP341 domain-separation prefix.  Keep this fix pinned
# here so a clean dependency download cannot silently reintroduce the bug.
changed |= patch_file(
    lib_root / "Hash.cpp",
    [
        (
            "    write(th, 32);\n"
            "    write(th, 32);\n"
            "}\n\n"
            "int tagged_hash",
            "    write(th, 32);\n"
            "    write(th, 32);\n"
            "    memzero(th, sizeof(th));\n"
            "}\n\n"
            "int tagged_hash",
        ),
        (
            "int tagged_hash(const char * tag, const uint8_t * data, size_t dataLen, uint8_t hash[32]){\n"
            "    TaggedHash th(tag);\n"
            "    return hashData(&th, data, dataLen, hash);\n"
            "}",
            "int tagged_hash(const char * tag, const uint8_t * data, size_t dataLen, uint8_t hash[32]){\n"
            "    TaggedHash th(tag);\n"
            "    th.write(data, dataLen);\n"
            "    return th.end(hash);\n"
            "}",
        ),
    ],
)

changed |= patch_file(
    lib_root / "HDWallet.cpp",
    [
        (
            "    pubKey = *this * GeneratorPoint;\n"
            "    pubKey.compressed = true;\n"
            "    return 1;\n"
            "}\n"
            "// int HDPrivateKey::fromSeed",
            "    pubKey = *this * GeneratorPoint;\n"
            "    pubKey.compressed = true;\n"
            "    memzero(raw, sizeof(raw));\n"
            "    memzero(key, sizeof(key));\n"
            "    return 1;\n"
            "}\n"
            "// int HDPrivateKey::fromSeed",
        ),
        (
            "    fromSeed(seed, sizeof(seed), net);\n"
            "    return 1;\n"
            "}\n"
            "#if USE_ARDUINO_STRING || USE_STD_STRING",
            "    const int result = fromSeed(seed, sizeof(seed), net);\n"
            "    memzero(seed, sizeof(seed));\n"
            "    memzero(u, sizeof(u));\n"
            "    memzero(ind, sizeof(ind));\n"
            "    memzero(salt, sizeof(salt));\n"
            "    return result;\n"
            "}\n"
            "#if USE_ARDUINO_STRING || USE_STD_STRING",
        ),
        (
            "    child.setSecret(secret);\n"
            "    memzero(secret, 32);\n"
            "    return child;\n"
            "}\n\n"
            "HDPrivateKey HDPrivateKey::hardenedChild",
            "    child.setSecret(secret);\n"
            "    memzero(secret, sizeof(secret));\n"
            "    memzero(raw, sizeof(raw));\n"
            "    memzero(data, sizeof(data));\n"
            "    memzero(hash, sizeof(hash));\n"
            "    memzero(sec, sizeof(sec));\n"
            "    return child;\n"
            "}\n\n"
            "HDPrivateKey HDPrivateKey::hardenedChild",
        ),
    ],
)

changed |= patch_file(
    lib_root / "Conversion.cpp",
    [
        (
            "        output[size+zeroCount-j-1] = BASE58_CHARS[reminder];\n"
            "    }\n"
            "    free(buffer);",
            "        output[size+zeroCount-j-1] = BASE58_CHARS[reminder];\n"
            "    }\n"
            "    memzero(buffer, bufferSize);\n"
            "    free(buffer);",
        ),
        (
            "    size_t l = toBase58(arr, arraySize+4, output, outputSize);\n"
            "    memzero(arr, arraySize+4); // secret should not stay in RAM",
            "    size_t l = toBase58(arr, arraySize+4, output, outputSize);\n"
            "    memzero(hash, sizeof(hash));\n"
            "    memzero(arr, arraySize+4); // secret should not stay in RAM",
        ),
    ],
)

changed |= patch_file(
    lib_root / "BitcoinCurve.cpp",
    [
        (
            "\t\tbn_write_be(&n, num);\n"
            "\t}\n"
            "\tbytes_parsed += bytes_read;",
            "\t\tbn_write_be(&n, num);\n"
            "        memzero(&n, sizeof(n));\n"
            "\t}\n"
            "\tbytes_parsed += bytes_read;",
        ),
        (
            "ECScalar ECScalar::operator+(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_addmod(&a, &b, &secp256k1.order);\n"
            "\tbn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "\treturn sum;\n"
            "}",
            "ECScalar ECScalar::operator+(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_addmod(&a, &b, &secp256k1.order);\n"
            "\tbn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "    memzero(&b, sizeof(b));\n"
            "\treturn sum;\n"
            "}",
        ),
        (
            "ECScalar ECScalar::operator+(const uint32_t& i) const{\n"
            "\tbignum256 a;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_addi(&a, i);\n"
            "\tbn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "\treturn sum;\n"
            "}",
            "ECScalar ECScalar::operator+(const uint32_t& i) const{\n"
            "\tbignum256 a;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_addi(&a, i);\n"
            "\tbn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "\treturn sum;\n"
            "}",
        ),
        (
            "ECScalar ECScalar::operator-() const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_subtract(&secp256k1.order, &a, &b);\n"
            "\tECScalar neg;\n"
            "\tbn_write_be(&b, neg.num);\n"
            "\treturn neg;\n"
            "}",
            "ECScalar ECScalar::operator-() const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_subtract(&secp256k1.order, &a, &b);\n"
            "\tECScalar neg;\n"
            "\tbn_write_be(&b, neg.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "    memzero(&b, sizeof(b));\n"
            "\treturn neg;\n"
            "}",
        ),
        (
            "ECScalar ECScalar::operator-(const uint32_t& i) const{\n"
            "    bignum256 a;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_subi(&a, i, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "\treturn sum;\n"
            "}",
            "ECScalar ECScalar::operator-(const uint32_t& i) const{\n"
            "    bignum256 a;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_subi(&a, i, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar sum;\n"
            "\tbn_write_be(&a, sum.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "\treturn sum;\n"
            "}",
        ),
        (
            "ECScalar ECScalar::operator*(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_multiply(&b, &a, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar mul;\n"
            "\tbn_write_be(&a, mul.num);\n"
            "\treturn mul;\n"
            "}",
            "ECScalar ECScalar::operator*(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_multiply(&b, &a, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar mul;\n"
            "\tbn_write_be(&a, mul.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "    memzero(&b, sizeof(b));\n"
            "\treturn mul;\n"
            "}",
        ),
        (
            "ECScalar ECScalar::operator/(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_inverse(&b, &secp256k1.order);\n"
            "\tbn_multiply(&b, &a, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar res;\n"
            "\tbn_write_be(&a, res.num);\n"
            "\treturn res;\n"
            "}",
            "ECScalar ECScalar::operator/(const ECScalar& other) const{\n"
            "    bignum256 a, b;\n"
            "\tbn_read_be(this->num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\tbn_inverse(&b, &secp256k1.order);\n"
            "\tbn_multiply(&b, &a, &secp256k1.order);\n"
            "    bn_mod(&a, &secp256k1.order);\n"
            "\tECScalar res;\n"
            "\tbn_write_be(&a, res.num);\n"
            "    memzero(&a, sizeof(a));\n"
            "    memzero(&b, sizeof(b));\n"
            "\treturn res;\n"
            "}",
        ),
        (
            "bool ECScalar::operator<(const ECScalar& other) const{\n"
            "\tbignum256 a,b;\n"
            "\tbn_read_be(num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "\treturn bn_is_less(&a, &b);\n"
            "}",
            "bool ECScalar::operator<(const ECScalar& other) const{\n"
            "\tbignum256 a,b;\n"
            "\tbn_read_be(num, &a);\n"
            "\tbn_read_be(other.num, &b);\n"
            "    const bool result = bn_is_less(&a, &b);\n"
            "    memzero(&a, sizeof(a));\n"
            "    memzero(&b, sizeof(b));\n"
            "\treturn result;\n"
            "}",
        ),
        (
            "\tif(point == GeneratorPoint){\n"
            "\t\tuint8_t pubkey[65];\n"
            "\t\tecdsa_get_public_key65(&secp256k1, num, pubkey);\n"
            "\t\tr.parse(pubkey, 65);\n"
            "\t}else{",
            "\tif(point == GeneratorPoint){\n"
            "\t\tuint8_t pubkey[65];\n"
            "\t\tecdsa_get_public_key65(&secp256k1, num, pubkey);\n"
            "\t\tr.parse(pubkey, 65);\n"
            "        memzero(pubkey, sizeof(pubkey));\n"
            "\t}else{",
        ),
        (
            "\t\tbn_write_be(&res.x, r.point);\n"
            "\t\tbn_write_be(&res.y, r.point+32);\n"
            "\t}\n"
            "\tr.compressed = point.compressed;\n"
            "\treturn r;",
            "\t\tbn_write_be(&res.x, r.point);\n"
            "\t\tbn_write_be(&res.y, r.point+32);\n"
            "        memzero(&d, sizeof(d));\n"
            "        memzero(&p, sizeof(p));\n"
            "        memzero(&res, sizeof(res));\n"
            "\t}\n"
            "    memzero(num, sizeof(num));\n"
            "\tr.compressed = point.compressed;\n"
            "\treturn r;",
        ),
    ],
)

if changed:
    print("AURORA: applied uBitcoin RAM hardening")
else:
    print("AURORA: uBitcoin RAM hardening already applied")
