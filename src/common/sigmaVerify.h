#include <iostream>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <utility>

class SigmaVerify
{
private:
    EC_POINT *publicKey;
    EC_GROUP *curve;
    BN_CTX *ctx;

    void sha256(EC_POINT *t, BIGNUM *challenge)
    {
        unsigned char buffer[SHA256_DIGEST_LENGTH];
        // unsigned char *t_serialized = EC_POINT_point2oct(curve, t, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
        size_t len = EC_POINT_point2oct(curve, t, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
        unsigned char *t_serialized = new unsigned char[len];
        len = EC_POINT_point2oct(curve, t, POINT_CONVERSION_COMPRESSED, t_serialized, len, NULL);

        SHA256(t_serialized, strlen((char *)t_serialized), buffer);
        BN_bin2bn(buffer, SHA256_DIGEST_LENGTH, challenge);
    }

public:
    SigmaVerify(EC_POINT *publicKey, EC_GROUP *curve)
    {
        this->publicKey = publicKey;
        this->curve = curve;
        this->ctx = BN_CTX_new();
    }

    ~SigmaVerify()
    {
        BN_CTX_free(this->ctx);
    }


    bool verify(EC_POINT *t, BIGNUM *response)
    {
        // Compute challenge as a hash of t
        BIGNUM *challenge = BN_new();
        sha256(t, challenge);

        // Compute expected t' = g^response / publicKey^challenge
        EC_POINT *inverted = EC_POINT_new(curve);
        EC_POINT_copy(inverted, publicKey);
        EC_POINT_mul(curve, inverted, NULL, inverted, challenge, ctx);
        EC_POINT_invert(curve, inverted, ctx);

        EC_POINT *expected_t = EC_POINT_new(curve);
        EC_POINT_mul(curve, expected_t, response, inverted, BN_value_one(), ctx);

        // Check if t' equals t
        bool isValid = (EC_POINT_cmp(curve, expected_t, t, ctx) == 0);

        BN_free(challenge);
        EC_POINT_free(expected_t);

        return isValid;
    }
};

// int main() {
//     // Initialize OpenSSL
//     ERR_load_crypto_strings();
//     OpenSSL_add_all_algorithms();

//     // Generate a random secret key
//     BIGNUM *secretKey = BN_new();
//     BN_rand(secretKey, 256, -1, 0);

//     // Compute the corresponding public key
//     EC_GROUP *curve = EC_GROUP_new_by_curve_name(NID_secp256k1);
//     EC_POINT *publicKey = EC_POINT_new(curve);
//     EC_POINT_mul(curve, publicKey, secretKey, NULL, NULL, NULL);

//     // Create a zero-knowledge prover and perform a proof
//     ZeroKnowledgeProver prover(secretKey, publicKey, curve);
//     auto [t, response] = prover.prove();

//     // Verify the proof
//     bool isValid = prover.verify(t, response);
//     std::cout << "Verification " << (isValid ? "succeeded" : "failed") << std::endl;

//     // Cleanup
//     EC_POINT_free(t);
//     BN_free(response);
//     BN_free(secretKey);
//     EC_POINT_free(publicKey);
//     EC_GROUP_free(curve);
//     EVP_cleanup();
//     ERR_free_strings();

//     return 0;
// }
