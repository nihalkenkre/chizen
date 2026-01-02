uint TausStep(inout uint z, int S1, int S2, int S3, uint M)
{
  uint b = (((z << S1) ^ z) >> S2);
  return z = (((z & M) << S3) ^ b);
}

uint LCGStep(inout uint z, uint A, uint C)
{
  return z = (A * z + C);
}

float HybridTaus(inout uvec4 z)
{
   return 2.3283064365387e-10 * (
    TausStep(z.x, 13, 19, 12, 4294967294) ^
    TausStep(z.y, 2, 25, 4, 4294967288) ^  
    TausStep(z.z, 3, 11, 17, 4294967280) ^
    LCGStep(z.w, 1664525, 1013904223)
   );
}