using System;
using System.IO;
using System.Text;
using System.Security.Cryptography;

namespace Xbox {

    public static class XContSig {

        public static uint XCONTSIG_TITLEID_SIZE = 0x4;
        public static uint XCONTSIG_OFFERID_SIZE = 0x8;
        public static uint XCONTSIG_KEY_SIZE = 0x10;
        public static uint XCONTSIG_CONTENT_KEY_SIZE = 0x14;
        public static uint XCONTSIG_SIGNATURE_SIZE = 0x14;

        public static bool XComputeContentSignatureKey(uint TitleId, byte[] XboxHDKey, out byte[] ContentSignatureKey) {
            return XComputeContentSignatureKey(BitConverter.GetBytes(TitleId), XboxHDKey, out ContentSignatureKey);
        }

        public static bool XComputeContentSignatureKey(byte[] TitleId, byte[] XboxHDKey, out byte[] ContentSignatureKey) {

            if (TitleId != null && TitleId.Length == XCONTSIG_TITLEID_SIZE && XboxHDKey != null && XboxHDKey.Length == XCONTSIG_KEY_SIZE) {

                using (HMACSHA1 sha1 = new HMACSHA1(XboxHDKey, true))
                    ContentSignatureKey = sha1.ComputeHash(TitleId);

                return true;
            }

            ContentSignatureKey = null;
            return false;
        }

        public static bool XapiComputeContentHeaderSignature(uint TitleId, byte[] XboxHDKey, byte[] buffer, int index, int length, out byte[] HeaderSignature) {
            return XapiComputeContentHeaderSignature(BitConverter.GetBytes(TitleId), XboxHDKey, buffer, index, length, out HeaderSignature);
        }

        public static bool XapiComputeContentHeaderSignature(byte[] TitleId, byte[] XboxHDKey, byte[] buffer, int index, int length, out byte[] HeaderSignature) {
            
            if (TitleId != null && TitleId.Length == XCONTSIG_TITLEID_SIZE && XboxHDKey != null && XboxHDKey.Length == XCONTSIG_KEY_SIZE &&
                buffer != null && buffer.Length > 0 && index >= 0 && index < buffer.Length && length <= buffer.Length && (index + length) <= buffer.Length) {

                byte[] ContentSignatureKey;
                if (XComputeContentSignatureKey(TitleId, XboxHDKey, out ContentSignatureKey)) {
                    using (HMACSHA1 sha1 = new HMACSHA1(ContentSignatureKey, true))
                        HeaderSignature = sha1.ComputeHash(buffer, index, length);

                    return true;
                }
            }

            HeaderSignature = null;
            return false;
        }

        public static bool XCreateContentSimple4831(uint TitleId, ulong OfferId, byte[] XboxHDKey, string ContentDisplayName, out byte[] ContentMetadata) {
            return XCreateContentSimple4831(BitConverter.GetBytes(TitleId), BitConverter.GetBytes(OfferId), XboxHDKey, ContentDisplayName, out ContentMetadata);
        }

        public static bool XCreateContentSimple4831(byte[] TitleId, byte[] OfferId, byte[] XboxHDKey, string ContentDisplayName, out byte[] ContentMetadata) {

            /*
                1.0.4831.9 style of ContentMeta data from the early version of XOnline
            */

            if (TitleId != null && TitleId.Length == XCONTSIG_TITLEID_SIZE && OfferId != null && OfferId.Length == XCONTSIG_OFFERID_SIZE && 
                XboxHDKey != null && XboxHDKey.Length == XCONTSIG_KEY_SIZE && !string.IsNullOrEmpty(ContentDisplayName)) {

                string LanguageSectionString = string.Format("[Default]\r\nName={0}\r\n", ContentDisplayName);
                byte[] LanguageSectionBuffer;
                byte[] LanguageSectionHash;

                using (MemoryStream stream = new MemoryStream()) {
                    stream.Write(new byte[] { 0xFF, 0xFE }, 0, 2);
                    stream.Write(Encoding.Unicode.GetBytes(LanguageSectionString), 0, Encoding.Unicode.GetByteCount(LanguageSectionString));

                    LanguageSectionBuffer = stream.ToArray();
                    stream.Close();
                }

                using (SHA1Managed sha1 = new SHA1Managed())
                    LanguageSectionHash = sha1.ComputeHash(LanguageSectionBuffer, 0, LanguageSectionBuffer.Length);

                ContentMetadata = new byte[(0x6C + LanguageSectionBuffer.Length)];
                using(MemoryStream stream = new MemoryStream(ContentMetadata)) {
                    stream.Position = 0x14;
                    stream.Write(BitConverter.GetBytes(0x46534358u), 0, 4); //magic
                    stream.Write(BitConverter.GetBytes((uint)0x6C), 0, 4); //header size
                    stream.Write(BitConverter.GetBytes((uint)1), 0, 4); //content type (?)
                    stream.Write(BitConverter.GetBytes((uint)1), 0, 4); //content flags
                    stream.Write(TitleId, 0, (int)XCONTSIG_TITLEID_SIZE); //title id
                    stream.Write(OfferId, 0, (int)XCONTSIG_OFFERID_SIZE); //offer id
                    stream.Position += 4; //skip publisher flags
                    stream.Write(BitConverter.GetBytes((uint)0x6C), 0, 4); //language section offset
                    stream.Write(BitConverter.GetBytes((uint)LanguageSectionBuffer.Length), 0, 4); //language section size
                    stream.Write(LanguageSectionHash, 0, LanguageSectionHash.Length); //language section hash
                    stream.Write(LanguageSectionBuffer, 0, LanguageSectionBuffer.Length); //language section
                    stream.Close();
                }

                byte[] HeaderSignature;
                if (XapiComputeContentHeaderSignature(TitleId, XboxHDKey, ContentMetadata, 0x14, (0x6C - 0x14), out HeaderSignature)) {

                    Array.Copy(HeaderSignature, 0, ContentMetadata, 0, XCONTSIG_SIGNATURE_SIZE);
                    return true;
                }

            }

            ContentMetadata = null;
            return false;
        }

    }

}
