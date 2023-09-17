#include "property.hpp"

#include "data.hpp"
#include "texture.hpp"

namespace Nif
{

    void NiTextureTransform::read(NIFStream* nif)
    {
        nif->read(mOffset);
        nif->read(mScale);
        nif->read(mRotation);
        mTransformMethod = static_cast<Method>(nif->get<uint32_t>());
        nif->read(mOrigin);
    }

    void NiTexturingProperty::Texture::read(NIFStream* nif)
    {
        nif->read(mEnabled);
        if (!mEnabled)
            return;

        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mSourceTexture.read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            nif->read(mClamp);
            nif->read(mFilter);
        }
        else
        {
            uint16_t flags;
            nif->read(flags);
            mClamp = flags & 0xF;
            mFilter = (flags >> 4) & 0xF;
        }

        if (nif->getVersion() >= NIFStream::generateVersion(20, 5, 0, 4))
            nif->read(mMaxAnisotropy);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            nif->read(mUVSet);

        // PS2 filtering settings
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
            nif->skip(4);

        if (nif->getVersion() <= NIFStream::generateVersion(4, 1, 0, 12))
            nif->skip(2); // Unknown

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mHasTransform);
            if (mHasTransform)
                mTransform.read(nif);
        }
    }

    void NiTexturingProperty::Texture::post(Reader& nif)
    {
        mSourceTexture.post(nif);
    }

    void NiTexturingProperty::read(NIFStream* nif)
    {
        Property::read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD
            || nif->getVersion() >= NIFStream::generateVersion(20, 1, 0, 2))
            nif->read(mFlags);
        if (nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 1))
            mApplyMode = static_cast<ApplyMode>(nif->get<uint32_t>());

        mTextures.resize(nif->get<uint32_t>());
        for (size_t i = 0; i < mTextures.size(); i++)
        {
            mTextures[i].read(nif);

            if (i == 5 && mTextures[5].mEnabled)
            {
                nif->read(mEnvMapLumaBias);
                nif->read(mBumpMapMatrix);
            }
            else if (i == 7 && mTextures[7].mEnabled && nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
                nif->read(mParallaxOffset);
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            mShaderTextures.resize(nif->get<uint32_t>());
            mShaderIds.resize(mShaderTextures.size());
            for (size_t i = 0; i < mShaderTextures.size(); i++)
            {
                mShaderTextures[i].read(nif);
                if (mShaderTextures[i].mEnabled)
                    nif->read(mShaderIds[i]);
            }
        }
    }

    void NiTexturingProperty::post(Reader& nif)
    {
        Property::post(nif);

        for (Texture& tex : mTextures)
            tex.post(nif);
        for (Texture& tex : mShaderTextures)
            tex.post(nif);
    }

    void BSSPParallaxParams::read(NIFStream* nif)
    {
        nif->read(mMaxPasses);
        nif->read(mScale);
    }

    void BSSPRefractionParams::read(NIFStream* nif)
    {
        nif->read(mStrength);
        nif->read(mPeriod);
    }

    void BSShaderProperty::read(NIFStream* nif)
    {
        NiShadeProperty::read(nif);

        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
        {
            nif->read(mType);
            nif->read(mShaderFlags1);
            nif->read(mShaderFlags2);
            nif->read(mEnvMapScale);
        }
    }

    void BSShaderLightingProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            nif->read(mClamp);
    }

    void BSShaderPPLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mTextureSet.read(nif);
        if (nif->getBethVersion() >= 15)
            mRefraction.read(nif);
        if (nif->getBethVersion() >= 25)
            mParallax.read(nif);
    }

    void BSShaderPPLightingProperty::post(Reader& nif)
    {
        BSShaderLightingProperty::post(nif);

        mTextureSet.post(nif);
    }

    void BSShaderNoLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mFilename = nif->getSizedString();
        if (nif->getBethVersion() >= 27)
            nif->read(mFalloffParams);
    }

    void BSSPLuminanceParams::read(NIFStream* nif)
    {
        nif->read(mLumEmittance);
        nif->read(mExposureOffset);
        nif->read(mFinalExposureMin);
        nif->read(mFinalExposureMax);
    };

    void BSSPWetnessParams::read(NIFStream* nif)
    {
        nif->read(mSpecScale);
        nif->read(mSpecPower);
        nif->read(mMinVar);
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mEnvMapScale);
        nif->read(mFresnelPower);
        nif->read(mMetalness);
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO4)
            nif->skip(4); // Unknown
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->skip(4); // Unknown
    };

    void BSSPMLParallaxParams::read(NIFStream* nif)
    {
        nif->read(mInnerLayerThickness);
        nif->read(mRefractionScale);
        nif->read(mInnerLayerTextureScale);
        nif->read(mEnvMapScale);
    };

    void BSSPTranslucencyParams::read(NIFStream* nif)
    {
        nif->read(mSubsurfaceColor);
        nif->read(mTransmissiveScale);
        nif->read(mTurbulence);
        nif->read(mThickObject);
        nif->read(mMixAlbedo);
    };

    void BSLightingShaderProperty::read(NIFStream* nif)
    {
        if (nif->getBethVersion() <= 139)
            nif->read(mType);

        BSShaderProperty::read(nif);

        if (nif->getBethVersion() <= 130)
        {
            nif->read(mShaderFlags1);
            nif->read(mShaderFlags2);
        }
        else if (nif->getBethVersion() >= 132)
        {
            uint32_t numShaderFlags1 = 0, numShaderFlags2 = 0;
            nif->read(numShaderFlags1);
            if (nif->getBethVersion() >= 152)
                nif->read(numShaderFlags2);
            nif->readVector(mShaderFlags1Hashes, numShaderFlags1);
            nif->readVector(mShaderFlags2Hashes, numShaderFlags2);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->read(mType);

        nif->read(mUVOffset);
        nif->read(mUVScale);
        mTextureSet.read(nif);
        nif->read(mEmissive);
        nif->read(mEmissiveMult);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mRootMaterial);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            nif->skip(4); // Unknown float

        nif->read(mClamp);
        nif->read(mAlpha);
        nif->read(mRefractionStrength);

        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mGlossiness);
        else
            nif->read(mSmoothness);

        nif->read(mSpecular);
        nif->read(mSpecStrength);

        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->readArray(mLightingEffects);
        else if (nif->getBethVersion() <= 139)
        {
            nif->read(mSubsurfaceRolloff);
            nif->read(mRimlightPower);
            if (mRimlightPower == std::numeric_limits<float>::max())
                nif->read(mBacklightPower);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
        {
            nif->read(mGrayscaleToPaletteScale);
            nif->read(mFresnelPower);
            mWetness.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            mLuminance.read(nif);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
        {
            nif->read(mDoTranslucency);
            if (mDoTranslucency)
                mTranslucency.read(nif);
            if (nif->get<uint8_t>() != 0)
            {
                mTextureArrays.resize(nif->get<uint32_t>());
                for (std::vector<std::string>& textureArray : mTextureArrays)
                    nif->getSizedStrings(textureArray, nif->get<uint32_t>());
            }
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
        {
            nif->skip(4); // Unknown
            nif->skip(4); // Unknown
            nif->skip(2); // Unknown
        }

        // TODO: consider separating this switch for pre-FO76 and FO76+
        switch (static_cast<BSLightingShaderType>(mType))
        {
            case BSLightingShaderType::ShaderType_EnvMap:
                if (nif->getBethVersion() <= 139)
                    nif->read(mEnvMapScale);
                if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
                {
                    nif->read(mUseSSR);
                    nif->read(mWetnessUseSSR);
                }
                break;
            case BSLightingShaderType::ShaderType_FaceTint:
                // Skin tint shader in FO76+
                if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
                    nif->read(mSkinTintColor);
                break;
            case BSLightingShaderType::ShaderType_SkinTint:
                if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
                    nif->read(mHairTintColor);
                else if (nif->getBethVersion() <= 130)
                    mSkinTintColor = { nif->get<osg::Vec3f>(), 1.f };
                else if (nif->getBethVersion() <= 139)
                    nif->read(mSkinTintColor);
                // Hair tint shader in FO76+
                else if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
                    nif->read(mHairTintColor);
                break;
            case BSLightingShaderType::ShaderType_HairTint:
                if (nif->getBethVersion() <= 139)
                    nif->read(mHairTintColor);
                break;
            case BSLightingShaderType::ShaderType_ParallaxOcc:
                mParallax.read(nif);
                break;
            case BSLightingShaderType::ShaderType_MultiLayerParallax:
                mMultiLayerParallax.read(nif);
                break;
            case BSLightingShaderType::ShaderType_SparkleSnow:
                nif->read(mSparkle);
                break;
            case BSLightingShaderType::ShaderType_EyeEnvmap:
                nif->read(mCubeMapScale);
                nif->read(mLeftEyeReflectionCenter);
                nif->read(mRightEyeReflectionCenter);
                break;
            default:
                break;
        }
    }

    void BSLightingShaderProperty::post(Reader& nif)
    {
        BSShaderProperty::post(nif);

        mTextureSet.post(nif);
    }

    void BSEffectShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        if (nif->getBethVersion() <= 130)
        {
            nif->read(mShaderFlags1);
            nif->read(mShaderFlags2);
        }
        else if (nif->getBethVersion() >= 132)
        {
            uint32_t numShaderFlags1 = 0, numShaderFlags2 = 0;
            nif->read(numShaderFlags1);
            if (nif->getBethVersion() >= 152)
                nif->read(numShaderFlags2);
            nif->readVector(mShaderFlags1Hashes, numShaderFlags1);
            nif->readVector(mShaderFlags2Hashes, numShaderFlags2);
        }

        nif->read(mUVOffset);
        nif->read(mUVScale);
        mSourceTexture = nif->getSizedString();

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            nif->skip(4); // Unknown

        uint32_t miscParams = nif->get<uint32_t>();
        mClamp = miscParams & 0xFF;
        mLightingInfluence = (miscParams >> 8) & 0xFF;
        mEnvMapMinLOD = (miscParams >> 16) & 0xFF;
        nif->read(mFalloffParams);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            nif->read(mRefractionPower);

        nif->read(mBaseColor);
        nif->read(mBaseColorScale);
        nif->read(mFalloffDepth);
        mGreyscaleTexture = nif->getSizedString();

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
        {
            mEnvMapTexture = nif->getSizedString();
            mNormalTexture = nif->getSizedString();
            mEnvMaskTexture = nif->getSizedString();
            nif->read(mEnvMapScale);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
        {
            nif->read(mRefractionPower);
            mReflectanceTexture = nif->getSizedString();
            mLightingTexture = nif->getSizedString();
            nif->read(mEmittanceColor);
            mEmitGradientTexture = nif->getSizedString();
            mLuminance.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
        {
            nif->skip(7); // Unknown bytes
            nif->skip(6 * sizeof(float)); // Unknown floats
            nif->skip(1); // Unknown byte
        }
    }

    void NiFogProperty::read(NIFStream* nif)
    {
        Property::read(nif);

        mFlags = nif->getUShort();
        mFogDepth = nif->getFloat();
        mColour = nif->getVector3();
    }

    void S_MaterialProperty::read(NIFStream* nif)
    {
        if (nif->getBethVersion() < 26)
        {
            ambient = nif->getVector3();
            diffuse = nif->getVector3();
        }
        specular = nif->getVector3();
        emissive = nif->getVector3();
        glossiness = nif->getFloat();
        alpha = nif->getFloat();
        if (nif->getBethVersion() >= 22)
            emissiveMult = nif->getFloat();
    }

    void NiVertexColorProperty::read(NIFStream* nif)
    {
        Property::read(nif);

        mFlags = nif->getUShort();
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            mVertexMode = static_cast<VertexMode>(nif->getUInt());
            mLightingMode = static_cast<LightMode>(nif->getUInt());
        }
        else
        {
            mVertexMode = static_cast<VertexMode>((mFlags >> 4) & 0x3);
            mLightingMode = static_cast<LightMode>((mFlags >> 3) & 0x1);
        }
    }

    void S_AlphaProperty::read(NIFStream* nif)
    {
        threshold = nif->getChar();
    }

    void S_StencilProperty::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            enabled = nif->getChar();
            compareFunc = nif->getInt();
            stencilRef = nif->getUInt();
            stencilMask = nif->getUInt();
            failAction = nif->getInt();
            zFailAction = nif->getInt();
            zPassAction = nif->getInt();
            drawMode = nif->getInt();
        }
        else
        {
            unsigned short flags = nif->getUShort();
            enabled = flags & 0x1;
            failAction = (flags >> 1) & 0x7;
            zFailAction = (flags >> 4) & 0x7;
            zPassAction = (flags >> 7) & 0x7;
            drawMode = (flags >> 10) & 0x3;
            compareFunc = (flags >> 12) & 0x7;
            stencilRef = nif->getUInt();
            stencilMask = nif->getUInt();
        }
    }

}
