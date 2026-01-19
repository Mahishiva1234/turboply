#include "turboply.hpp"
#include <format>

namespace turboply::ext {

void insertGeoPlyHeadr(
    const std::filesystem::path& filename,
    const std::string& label,
    const int srid,
    const std::array<double, 6>& bbox,
    const std::array<double, 3>& offset,
    const std::array<double, 3>& scale
) {
	std::ifstream fin(filename, std::ios::binary);
	std::string data((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
	fin.close();

	std::size_t start_pos = data.find("format ");
	std::size_t insert_pos = data.find("element ", start_pos);

	std::string comment_block;
	comment_block += std::format("comment {}\n", label);
	comment_block += std::format("comment SRID:{}\n", srid);
	comment_block += std::format(
		"comment BoundingBox({:.8f} {:.8f} {:.8f} {:.8f} {:.8f} {:.8f})\n",
		bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]);
	comment_block += std::format("comment Offset({:.8f} {:.8f} {:.8f})\n",
		offset[0], offset[1], offset[2]);
	comment_block += std::format("comment Scale({:.4f} {:.4f} {:.4f})\n",
		scale[0], scale[1], scale[2]);

	data.insert(insert_pos, comment_block);

	std::ofstream fout(filename, std::ios::binary | std::ios::trunc);
	fout.write(data.data(), static_cast<std::streamsize>(data.size()));
	fout.close();
}

bool parseGeoPlyHeadr(
	const std::vector<std::string>& comments,
	std::string& label, int& srid,
	std::array<double, 6>& bbox,
	std::array<double, 3>& offset,
	std::array<double, 3>& scale
) {
	bool result = true;
	if (comments.size()) {
		label = comments.size() > 0 ? comments.at(0) : "";
		result &= comments.size() > 1 &&
			sscanf_s(comments.at(1).c_str(), "SRID:%d", &srid) == 1;
		result &= comments.size() > 2 &&
			sscanf_s(comments.at(2).c_str(), "BoundingBox(%lf %lf %lf %lf %lf %lf)",
				&bbox[0], &bbox[1], &bbox[2], &bbox[3], &bbox[4], &bbox[5]) == 6;
		result &= comments.size() > 4 &&
			sscanf_s(comments.at(3).c_str(), "Offset(%lf %lf %lf)",
				&offset[0], &offset[1], &offset[2]) == 3 &&
			sscanf_s(comments.at(4).c_str(), "Scale(%lf %lf %lf)",
				&scale[0], &scale[1], &scale[2]) == 3;
	}
	else
		result = false;

	return result;
}

bool fetchGeoPlyHeadr(
	const std::filesystem::path& filename,
	std::string& label, int& srid,
	std::array<double, 6>& bbox,
	std::array<double, 3>& offset,
	std::array<double, 3>& scale
) {
	PlyFileReader reader(filename);
	reader.parseHeader();
	return parseGeoPlyHeadr(reader.getComments(), label, srid, bbox, offset, scale);
}

//////////////////////////////////////////////////////////////////////////

constexpr const char* PLY_TEXTURE_FILE = "TextureFile";

void GeoPlyFileWriter::addHeader(
	const std::string& label, int srid, const std::array<double, 6>& bbox,
	const std::array<double, 3>& offset, const std::array<double, 3>& scale
) {
	addComment(label);
	addComment(std::format("SRID:{0}", srid));
	addComment(std::format("BoundingBox({:.8f} {:.8f} {:.8f} {:.8f} {:.8f} {:.8f})", 
		bbox[0], bbox[1], bbox[2], bbox[3], bbox[4], bbox[5]));
	addComment(std::format("Offset({:.8f} {:.8f} {:.8f})",
		offset[0], offset[1], offset[2]));
	addComment(std::format("Scale({:.4f} {:.4f} {:.4f})",
		scale[0], scale[1], scale[2]));
}

void GeoPlyFileWriter::writeTexturePath(const std::vector<std::string>& textures) {
	for (auto& t : textures)
		addComment(std::format("{} {}", PLY_TEXTURE_FILE, t));
}

bool GeoPlyFileReader::parseHeader(
	std::string& label, int& srid, std::array<double, 6>& bbox,
	std::array<double, 3>& offset, std::array<double, 3>& scale
) {
	parseHeader();
	auto comments = getComments();
	return parseGeoPlyHeadr(comments, label, srid, bbox, offset, scale);
}

bool GeoPlyFileReader::parseTexturePath(std::vector<std::string>& textures) {
	if (this->_comments.size() > 5) {
		for (size_t i = 5; i < this->_comments.size(); i++) {
			size_t pos = this->_comments.at(i).find(PLY_TEXTURE_FILE);

			if (pos != std::string::npos) {
				constexpr std::size_t LEN = std::string_view(PLY_TEXTURE_FILE).size() + 1;
				textures.emplace_back(this->_comments.at(i).substr(LEN, this->_comments.at(i).size() - LEN));
			}
		}

		return textures.size() > 0;
	}

	return false;
}

}