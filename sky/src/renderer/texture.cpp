#include "texture.h"

namespace sky
{
Texture2D::Texture2D(Buffer data, TextureSpecification specs)
	: m_data(data), m_specs(specs)
{
}
} // namespace sky