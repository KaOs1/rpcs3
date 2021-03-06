#include "stdafx.h"
#include "rsx_gl_texture.h"
#include "gl_helpers.h"
#include "../GCM.h"
#include "../RSXThread.h"
#include "../RSXTexture.h"
#include "../rsx_utils.h"
#include "../Common/TextureUtils.h"

namespace
{
	GLenum get_sized_internal_format(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return GL_R8;
		case CELL_GCM_TEXTURE_A1R5G5B5: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_A4R4G4B4: return GL_RGBA4;
		case CELL_GCM_TEXTURE_R5G6B5: return GL_RGB565;
		case CELL_GCM_TEXTURE_A8R8G8B8: return GL_RGBA8;
		case CELL_GCM_TEXTURE_G8B8: return GL_RG8;
		case CELL_GCM_TEXTURE_R6G5B5: return GL_RGB565;
		case CELL_GCM_TEXTURE_DEPTH24_D8: return GL_DEPTH_COMPONENT24;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return GL_DEPTH_COMPONENT24;
		case CELL_GCM_TEXTURE_DEPTH16: return GL_DEPTH_COMPONENT16;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return GL_DEPTH_COMPONENT16;
		case CELL_GCM_TEXTURE_X16: return GL_R16;
		case CELL_GCM_TEXTURE_Y16_X16: return GL_RG16;
		case CELL_GCM_TEXTURE_R5G5B5A1: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return GL_RGBA16F;
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return GL_RGBA32F;
		case CELL_GCM_TEXTURE_X32_FLOAT: return GL_R32F;
		case CELL_GCM_TEXTURE_D1R5G5B5: return GL_RGB5_A1;
		case CELL_GCM_TEXTURE_D8R8G8B8: return GL_RGBA8;
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return GL_RG16F;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		throw EXCEPTION("Compressed or unknown texture format %x", texture_format);
	}


	std::tuple<GLenum, GLenum> get_format_type(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return std::make_tuple(GL_RED, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_A1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_A4R4G4B4: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
		case CELL_GCM_TEXTURE_R5G6B5: return std::make_tuple(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		case CELL_GCM_TEXTURE_A8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case CELL_GCM_TEXTURE_G8B8: return std::make_tuple(GL_RG, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_R6G5B5: return std::make_tuple(GL_RGBA, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_DEPTH24_D8: return std::make_tuple(GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE);
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case CELL_GCM_TEXTURE_DEPTH16: return std::make_tuple(GL_DEPTH_COMPONENT, GL_SHORT);
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: return std::make_tuple(GL_DEPTH_COMPONENT, GL_FLOAT);
		case CELL_GCM_TEXTURE_X16: return std::make_tuple(GL_RED, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_Y16_X16: return std::make_tuple(GL_RG, GL_UNSIGNED_SHORT);
		case CELL_GCM_TEXTURE_R5G5B5A1: return std::make_tuple(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return std::make_tuple(GL_RGBA, GL_HALF_FLOAT);
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return std::make_tuple(GL_RGBA, GL_FLOAT);
		case CELL_GCM_TEXTURE_X32_FLOAT: return std::make_tuple(GL_RED, GL_FLOAT);
		case CELL_GCM_TEXTURE_D1R5G5B5: return std::make_tuple(GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		case CELL_GCM_TEXTURE_D8R8G8B8: return std::make_tuple(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8);
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return std::make_tuple(GL_RG, GL_HALF_FLOAT);
		}
		throw EXCEPTION("Compressed or unknown texture format %x", texture_format);
	}

	bool is_compressed_format(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			return false;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			return true;
		}
		throw EXCEPTION("Unknown format %x", texture_format);
	}

	bool requires_unpack_byte(u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
			return true;
		}
		return false;
	}

	std::array<GLenum, 4> get_swizzle_remap(u32 texture_format)
	{
		// NOTE: This must be in ARGB order in all forms below.
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8: return{ GL_RED, GL_RED, GL_RED, GL_RED };
		case CELL_GCM_TEXTURE_A4R4G4B4: return { GL_BLUE, GL_ALPHA, GL_RED, GL_GREEN };
		case CELL_GCM_TEXTURE_G8B8: return { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		case CELL_GCM_TEXTURE_X16: return { GL_RED, GL_ONE, GL_RED, GL_ONE };
		case CELL_GCM_TEXTURE_Y16_X16: return { GL_GREEN, GL_RED, GL_GREEN, GL_RED };
		case CELL_GCM_TEXTURE_X32_FLOAT: return { GL_RED, GL_ONE, GL_ONE, GL_ONE };
		case CELL_GCM_TEXTURE_D1R5G5B5: return { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
		case CELL_GCM_TEXTURE_D8R8G8B8: return { GL_ONE, GL_RED, GL_GREEN, GL_BLUE };
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT: return { GL_RED, GL_GREEN, GL_RED, GL_GREEN };
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			return{ GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE };
		}
		throw EXCEPTION("Unknown format %x", texture_format);
	}
}

namespace rsx
{
	namespace gl
	{
		static const int gl_tex_min_filter[] =
		{
			GL_NEAREST, // unused
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST_MIPMAP_NEAREST,
			GL_LINEAR_MIPMAP_NEAREST,
			GL_NEAREST_MIPMAP_LINEAR,
			GL_LINEAR_MIPMAP_LINEAR,
			GL_NEAREST, // CELL_GCM_TEXTURE_CONVOLUTION_MIN
		};

		static const int gl_tex_mag_filter[] =
		{
			GL_NEAREST, // unused
			GL_NEAREST,
			GL_LINEAR,
			GL_NEAREST, // unused
			GL_LINEAR  // CELL_GCM_TEXTURE_CONVOLUTION_MAG
		};

		static const int gl_tex_zfunc[] =
		{
			GL_NEVER,
			GL_LESS,
			GL_EQUAL,
			GL_LEQUAL,
			GL_GREATER,
			GL_NOTEQUAL,
			GL_GEQUAL,
			GL_ALWAYS,
		};

		void texture::create()
		{
			if (m_id)
			{
				remove();
			}

			glGenTextures(1, &m_id);
		}

		int texture::gl_wrap(int wrap)
		{
			switch (wrap)
			{
			case CELL_GCM_TEXTURE_WRAP: return GL_REPEAT;
			case CELL_GCM_TEXTURE_MIRROR: return GL_MIRRORED_REPEAT;
			case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
			case CELL_GCM_TEXTURE_BORDER: return GL_CLAMP_TO_BORDER;
			case CELL_GCM_TEXTURE_CLAMP: return GL_CLAMP;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
			case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return GL_MIRROR_CLAMP_EXT;
			}

			LOG_ERROR(RSX, "Texture wrap error: bad wrap (%d).", wrap);
			return GL_REPEAT;
		}

		float texture::max_aniso(int aniso)
		{
			switch (aniso)
			{
			case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12.0f;
			case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16.0f;
			}

			LOG_ERROR(RSX, "Texture anisotropy error: bad max aniso (%d).", aniso);
			return 1.0f;
		}

		u16 texture::get_pitch_modifier(u32 format)
		{
			switch (format)
			{
			case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
			case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			default:
				LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
				return 0;
			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
				return 4;
			case CELL_GCM_TEXTURE_B8:
				return 1;
			case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
			case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
				return 0;
			case CELL_GCM_TEXTURE_A1R5G5B5:
			case CELL_GCM_TEXTURE_A4R4G4B4:
			case CELL_GCM_TEXTURE_R5G6B5:
			case CELL_GCM_TEXTURE_G8B8:
			case CELL_GCM_TEXTURE_R6G5B5:
			case CELL_GCM_TEXTURE_DEPTH16:
			case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			case CELL_GCM_TEXTURE_X16:
			case CELL_GCM_TEXTURE_R5G5B5A1:
			case CELL_GCM_TEXTURE_D1R5G5B5:
				return 2;
			case CELL_GCM_TEXTURE_A8R8G8B8:
			case CELL_GCM_TEXTURE_X32_FLOAT:
			case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			case CELL_GCM_TEXTURE_D8R8G8B8:
			case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			case CELL_GCM_TEXTURE_DEPTH24_D8:
			case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			case CELL_GCM_TEXTURE_Y16_X16:
				return 4;
			case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
				return 8;
			case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
				return 16;
			}
		}

		bool texture::mandates_expansion(u32 format)
		{
			/**
			 * If a texture behaves differently when uploaded directly vs when uploaded via texutils methods, it should be added here.
			 */
			if (format == CELL_GCM_TEXTURE_A1R5G5B5)
				return true;
			
			return false;
		}

		void texture::init(int index, rsx::texture& tex)
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());

			//We can't re-use texture handles if using immutable storage
			if (m_id) remove();
			create();

			glActiveTexture(GL_TEXTURE0 + index);
			bind();

			u32 full_format = tex.format();

			u32 format = full_format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
			bool is_swizzled = !!(~full_format & CELL_GCM_TEXTURE_LN);

			::gl::pixel_pack_settings().apply();
			::gl::pixel_unpack_settings().apply();

			u32 aligned_pitch = tex.pitch();

			size_t texture_data_sz = get_placed_texture_storage_size(tex, 256);
			std::vector<gsl::byte> data_upload_buf(texture_data_sz);
			u32 block_sz = get_pitch_modifier(format);

			const std::vector<rsx_subresource_layout> &input_layouts = get_subresources_layout(tex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			glTexStorage2D(m_target, tex.mipmap(), get_sized_internal_format(format), tex.width(), tex.height());

			if (!is_compressed_format(format))
			{
				const auto &format_type = get_format_type(format);
				GLint mip_level = 0;
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					upload_texture_subresource(data_upload_buf, layout, format, is_swizzled, 4);
					glTexSubImage2D(m_target, mip_level++, 0, 0, layout.width_in_block, layout.height_in_block, std::get<0>(format_type), std::get<1>(format_type), data_upload_buf.data());
				}
			}
			else
			{

				GLint mip_level = 0;
				for (const rsx_subresource_layout &layout : input_layouts)
				{
					u32 size = layout.width_in_block * layout.height_in_block * ((format == CELL_GCM_TEXTURE_COMPRESSED_DXT1) ? 8 : 16);
					glCompressedTexSubImage2D(m_target, mip_level++, 0, 0, layout.width_in_block * 4, layout.height_in_block * 4, get_sized_internal_format(format), size, layout.data.data());
				}
			}

			const std::array<GLenum, 4>& glRemap = get_swizzle_remap(format);

			glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, tex.mipmap() - 1);

			if (format != CELL_GCM_TEXTURE_B8 && format != CELL_GCM_TEXTURE_X16 && format != CELL_GCM_TEXTURE_X32_FLOAT)
			{
				u8 remap_a = tex.remap() & 0x3;
				u8 remap_r = (tex.remap() >> 2) & 0x3;
				u8 remap_g = (tex.remap() >> 4) & 0x3;
				u8 remap_b = (tex.remap() >> 6) & 0x3;

				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[remap_a]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[remap_r]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[remap_g]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[remap_b]);
			}
			else
			{

				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_A, glRemap[0]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_R, glRemap[1]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_G, glRemap[2]);
				glTexParameteri(m_target, GL_TEXTURE_SWIZZLE_B, glRemap[3]);
			}

			glTexParameteri(m_target, GL_TEXTURE_WRAP_S, gl_wrap(tex.wrap_s()));
			glTexParameteri(m_target, GL_TEXTURE_WRAP_T, gl_wrap(tex.wrap_t()));
			glTexParameteri(m_target, GL_TEXTURE_WRAP_R, gl_wrap(tex.wrap_r()));

			glTexParameteri(m_target, GL_TEXTURE_COMPARE_FUNC, gl_tex_zfunc[tex.zfunc()]);

			glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, (GLint)tex.bias());
			glTexParameteri(m_target, GL_TEXTURE_MIN_LOD, (tex.min_lod() >> 8));
			glTexParameteri(m_target, GL_TEXTURE_MAX_LOD, (tex.max_lod() >> 8));

			int min_filter = gl_tex_min_filter[tex.min_filter()];
			
			if (min_filter != GL_LINEAR && min_filter != GL_NEAREST)
			{
				if (tex.mipmap() <= 1 || m_target == GL_TEXTURE_RECTANGLE)
				{
					LOG_WARNING(RSX, "Texture %d, target 0x%X, requesting mipmap filtering without any mipmaps set!", m_id, m_target);
					min_filter = GL_LINEAR;
				}
			}

			glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, min_filter);
			glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, gl_tex_mag_filter[tex.mag_filter()]);
			glTexParameterf(m_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso(tex.max_aniso()));
		}

		void texture::bind()
		{
			glBindTexture(m_target, m_id);
		}

		void texture::unbind()
		{
			glBindTexture(m_target, 0);
		}

		void texture::remove()
		{
			if (m_id)
			{
				glDeleteTextures(1, &m_id);
				m_id = 0;
			}
		}

		u32 texture::id() const
		{
			return m_id;
		}
	}
}
