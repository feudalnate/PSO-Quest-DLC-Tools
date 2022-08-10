using System;
using System.Security.Cryptography;

namespace Xbox {

    public static class XCalcSig {

        public static uint XCALCSIG_KEY_SIZE = 0x10;
        public static uint XCALCSIG_SIGNATURE_SIZE = 0x14;
        public static uint XCALCSIG_HASH_SIZE = 0x14;
        public static uint XONLINE_SIGNATURE_SIZE = 0x64;

        private static readonly byte[] XboxCERTKey =
            { 0x5C, 0x07, 0x33, 0xAE, 0x04, 0x01, 0xF7, 0xE8, 0xBA, 0x79, 0x93, 0xFD, 0xCD, 0x2F, 0x1F, 0xE0 };

        public struct XCalcSigContext {
            public XCalcSigFlag Flag;
            public byte[] SignatureKey;
            public byte[] XboxHDKey;
            public HashAlgorithm SHAContext;
        }

        public enum XCalcSigFlag : uint {
            SAVE_GAME = 0x00000000,
            NON_ROAMABLE = 0x00000001,
            CONTENT = 0x00000002,
            DIGEST = 0x00000004,
            ONLINE = 0x00000008
        };

        public static uint XCalculateSignatureGetSize(XCalcSigFlag flag) {

            if (flag <= XCalcSigFlag.CONTENT || flag == XCalcSigFlag.DIGEST)
                return XCALCSIG_SIGNATURE_SIZE;

            if (flag == XCalcSigFlag.ONLINE)
                return XONLINE_SIGNATURE_SIZE;

            return 0;
        }

        public static bool XCalculateSignatureBegin(out XCalcSigContext context, XCalcSigFlag flag) {

            if (flag == XCalcSigFlag.CONTENT || flag == XCalcSigFlag.DIGEST || flag == XCalcSigFlag.ONLINE)
                return XCalculateSignatureBegin(out context, flag, null, null);

            context = default(XCalcSigContext);
            return false;
        }

        public static bool XCalculateSignatureBegin(out XCalcSigContext context, XCalcSigFlag flag, byte[] TitleSignatureKey) {

            if (flag == XCalcSigFlag.SAVE_GAME || flag == XCalcSigFlag.CONTENT || flag == XCalcSigFlag.DIGEST || flag == XCalcSigFlag.ONLINE)
                return XCalculateSignatureBegin(out context, flag, TitleSignatureKey, null);

            context = default(XCalcSigContext);
            return false;
        }

        public static bool XCalculateSignatureBegin(out XCalcSigContext context, XCalcSigFlag flag, byte[] TitleSignatureKey, byte[] XboxHDKey) {

            if (flag == XCalcSigFlag.SAVE_GAME || flag == XCalcSigFlag.NON_ROAMABLE) {
                if (TitleSignatureKey != null && TitleSignatureKey.Length == XCALCSIG_KEY_SIZE) {

                    if (flag == XCalcSigFlag.NON_ROAMABLE && XboxHDKey == null || XboxHDKey.Length != XCALCSIG_KEY_SIZE)
                        goto exit;

                    byte[] tempHash;

                    context = new XCalcSigContext();
                    context.Flag = flag;
                    context.SignatureKey = new byte[XCALCSIG_KEY_SIZE];
                    context.XboxHDKey = (flag == XCalcSigFlag.NON_ROAMABLE ? XboxHDKey : null);

                    using (HMACSHA1 sha1 = new HMACSHA1(XboxCERTKey, true))
                        tempHash = sha1.ComputeHash(TitleSignatureKey);

                    Array.Copy(tempHash, 0, context.SignatureKey, 0, XCALCSIG_KEY_SIZE);
                    context.SHAContext = new HMACSHA1(context.SignatureKey, true);

                    return true;

                }
            }
            else if (flag == XCalcSigFlag.CONTENT || flag == XCalcSigFlag.DIGEST) {

                context = new XCalcSigContext() {
                    Flag = flag,
                    SignatureKey = null,
                    XboxHDKey = null,
                    SHAContext = new SHA1Managed()
                };

                return true;
            }
            else if (flag == XCalcSigFlag.ONLINE) {
                //Unsupported
            }

exit:
            context = default(XCalcSigContext);
            return false;
        }

        public static bool XCalculateSignatureUpdate(ref XCalcSigContext context, byte[] buffer, int index, int length) {

            if (!Equals(context, default(XCalcSigContext)) && !Equals(context, null) && buffer != null && buffer.Length > 0 &&
                index >= 0 && index < buffer.Length && length <= buffer.Length && (index + length) <= buffer.Length) {

                context.SHAContext.TransformBlock(buffer, index, length, null, 0);
                return true;
            }

            return false;
        }

        public static bool XCalculateSignatureEnd(ref XCalcSigContext context, out byte[] signature) {

            bool result = false;
            signature = null;

            if (!Equals(context, default(XCalcSigContext)) && !Equals(context, null)) {

                if (context.Flag == XCalcSigFlag.SAVE_GAME || context.Flag == XCalcSigFlag.CONTENT || context.Flag == XCalcSigFlag.DIGEST) {

                    context.SHAContext.TransformFinalBlock(new byte[0], 0, 0);

                    signature = new byte[XCALCSIG_SIGNATURE_SIZE];
                    Array.Copy(context.SHAContext.Hash, 0, signature, 0, XCALCSIG_SIGNATURE_SIZE);

                    result = true;
                }
                else if (context.Flag == XCalcSigFlag.NON_ROAMABLE && context.XboxHDKey != null && context.XboxHDKey.Length == XCALCSIG_KEY_SIZE) {

                    context.SHAContext.TransformFinalBlock(new byte[0], 0, 0);

                    signature = new byte[XCALCSIG_SIGNATURE_SIZE];
                    Array.Copy(context.SHAContext.Hash, 0, signature, 0, XCALCSIG_SIGNATURE_SIZE);

                    byte[] tempHash;
                    using (HMACSHA1 sha1 = new HMACSHA1(context.XboxHDKey, true))
                        tempHash = sha1.ComputeHash(signature);

                    Array.Copy(tempHash, 0, signature, 0, XCALCSIG_SIGNATURE_SIZE);

                    result = true;
                }
                else if (context.Flag == XCalcSigFlag.ONLINE) {
                    //Unsupported
                }

                context.SHAContext.Clear();
                context.SHAContext = null;
                context.SignatureKey = null;
                context.XboxHDKey = null;
                context = default(XCalcSigContext);
            }

            return result;
        }

    }

}
