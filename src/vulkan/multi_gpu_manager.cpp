#include "multi_gpu_manager.h"
#include "path_tracer.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cstdio>

namespace vkpt {

// SPIR-V Compute Shader Bytecode (kompiliert mit: glslangValidator -V shader.comp -o shader.spv)
// Dieser Shader wurde aus /workspace/src/shaders/compute_shader.spv geladen
static const std::vector<uint32_t>& getCompiledShaderCode() {
    static const std::vector<uint32_t> computeShaderCode = {
119734787, 65536, 524299, 413, 0, 131089, 1, 393227, 1, 1280527431, 1685353262, 808793134, 0, 196622, 0, 1, 393231, 5, 4, 1852399981, 0, 39, 393232, 4, 17, 16, 16, 1, 196611, 2, 450, 262149, 4, 1852399981, 0, 327685, 8, 1684955506, 1953392981, 40, 327685, 12, 1684955506, 1634692166, 10356, 458757, 16, 1684955506, 1850305903, 1953066581, 1701343315, 2647410, 720901, 30, 1400138088, 1919248496, 1719019621, 1719024435, 828783411, 993093179, 1983590758, 1966814054, 15153, 262149, 23, 1734963823, 28265, 327685, 24, 1701996900, 1869182051, 110, 262149, 25, 1852403060, 0, 262149, 26, 2019650932, 0, 196613, 27, 116, 262149, 28, 1836216174, 27745, 327685, 29, 1702125933, 1818323314, 7889993, 393221, 34, 1131701607, 1919904879, 993097000, 0, 327685, 33, 1702125933, 1818323314, 7889993, 196613, 36, 7890025, 524293, 39, 1197436007, 1633841004, 1986939244, 1952539503, 1231974249, 68, 393221, 47, 1718185557, 1114468975, 1701209717, 114, 327686, 47, 0, 1952737655, 104, 327686, 47, 1, 1734960488, 29800, 393222, 47, 2, 1148739949, 1752461413, 0, 327686, 47, 3, 1684366707, 0, 458758, 47, 4, 1701667171, 1917804914, 1852401513, 0, 393222, 47, 5, 1701667171, 1766089074, 114, 393222, 47, 6, 1701667171, 1884643698, 0, 262150, 47, 7, 7761766, 327685, 49, 1718185589, 1936552559, 0, 262149, 57, 1952543859, 101, 196613, 87, 112, 196613, 106, 105, 393221, 115, 1701343315, 1967285618, 1919247974, 0, 327686, 115, 0, 1635017060, 0, 262149, 117, 1701343347, 7562610, 262149, 125, 1701343347, 25970, 262149, 130, 1953391971, 29285, 262149, 133, 1768186226, 29557, 196613, 137, 25455, 196613, 141, 98, 196613, 145, 99, 393221, 153, 1668508004, 1768778098, 1953390958, 0, 262149, 164, 1886217588, 0, 393221, 198, 1702125901, 1818323314, 1717990722, 29285, 327686, 198, 0, 1635017060, 0, 327685, 200, 1702125933, 1818323314, 115, 262149, 210, 1148477805, 6386785, 196613, 224, 120, 196613, 227, 121, 196613, 245, 117, 196613, 254, 118, 262149, 262, 1701868385, 29795, 327685, 270, 1215193460, 1181117537, 30319, 327685, 278, 1769107304, 1953394554, 27745, 327685, 283, 1953654134, 1818321769, 0, 262149, 286, 1148805490, 29289, 262149, 304, 1869377379, 114, 327685, 305, 1869768820, 1885890421, 29813, 262149, 307, 1734963823, 28265, 327685, 310, 1701996900, 1869182051, 110, 262149, 312, 1953523044, 104, 196613, 325, 116, 262149, 326, 1836216174, 27745, 327685, 327, 1702125933, 1818323314, 7889993, 262149, 328, 1634886000, 109, 262149, 330, 1634886000, 109, 262149, 332, 1634886000, 109, 262149, 333, 1634886000, 109, 262149, 334, 1634886000, 109, 262149, 335, 1634886000, 109, 262149, 336, 1634886000, 109, 327685, 349, 1349806440, 1953393007, 0, 262149, 355, 1700949089, 28516, 262149, 356, 1634886000, 109, 327685, 359, 1952539507, 1148347764, 29289, 196613, 364, 112, 327685, 394, 1634625894, 1819231084, 29295, 327685, 402, 1886680431, 1833530485, 6645601, 262215, 39, 11, 28, 327752, 47, 0, 35, 0, 327752, 47, 1, 35, 4, 327752, 47, 2, 35, 8, 327752, 47, 3, 35, 12, 327752, 47, 4, 35, 16, 327752, 47, 5, 35, 32, 327752, 47, 6, 35, 48, 327752, 47, 7, 35, 60, 196679, 47, 2, 262215, 49, 34, 0, 262215, 49, 33, 0, 262215, 114, 6, 16, 327752, 115, 0, 35, 0, 196679, 115, 3, 262215, 117, 34, 0, 262215, 117, 33, 1, 262215, 197, 6, 16, 327752, 198, 0, 35, 0, 196679, 198, 3, 262215, 200, 34, 0, 262215, 200, 33, 2, 262215, 402, 34, 0, 262215, 402, 33, 3, 262215, 412, 11, 25, 131091, 2, 196641, 3, 2, 262165, 6, 32, 0, 196641, 7, 6, 196630, 10, 32, 196641, 11, 10, 262167, 14, 10, 3, 196641, 15, 14, 262176, 18, 7, 14, 262176, 19, 7, 10, 262176, 20, 7, 6, 131092, 21, 655393, 22, 21, 18, 18, 19, 19, 19, 18, 20, 262177, 32, 14, 20, 262167, 37, 6, 3, 262176, 38, 1, 37, 262203, 38, 39, 1, 262187, 6, 40, 0, 262176, 41, 1, 6, 262187, 6, 44, 1, 655390, 47, 6, 6, 6, 6, 14, 14, 14, 10, 262176, 48, 2, 47, 262203, 48, 49, 2, 262165, 50, 32, 1, 262187, 50, 51, 0, 262176, 52, 2, 6, 262187, 6, 59, 7919, 262187, 50, 61, 3, 262187, 6, 64, 104729, 262187, 6, 68, 1664525, 262187, 6, 70, 1013904223, 262187, 10, 77, 1333788672, 196649, 21, 86, 262187, 10, 92, 1073741824, 262187, 10, 94, 1065353216, 262167, 113, 10, 4, 196637, 114, 113, 196638, 115, 114, 262176, 116, 2, 115, 262203, 116, 117, 2, 262187, 50, 120, 4, 262176, 124, 7, 113, 262176, 127, 2, 113, 262187, 6, 134, 3, 262187, 10, 160, 0, 262187, 50, 191, 1, 196650, 21, 193, 196637, 197, 113, 196638, 198, 197, 262176, 199, 2, 198, 262203, 199, 200, 2, 393260, 14, 208, 160, 160, 160, 262187, 6, 218, 2, 262187, 10, 248, 1056964608, 262187, 50, 271, 7, 262176, 272, 2, 10, 262187, 50, 287, 5, 262176, 288, 2, 14, 393260, 14, 306, 94, 94, 94, 262187, 50, 319, 2, 262187, 10, 323, 981668463, 262187, 10, 324, 1176256512, 589849, 400, 10, 1, 0, 0, 0, 2, 1, 262176, 401, 0, 400, 262203, 401, 402, 0, 262167, 408, 50, 2, 262187, 6, 411, 16, 393260, 37, 412, 411, 411, 44, 327734, 2, 4, 0, 3, 131320, 5, 262203, 20, 224, 7, 262203, 20, 227, 7, 262203, 19, 245, 7, 262203, 19, 254, 7, 262203, 19, 262, 7, 262203, 18, 270, 7, 262203, 18, 283, 7, 262203, 18, 286, 7, 262203, 18, 304, 7, 262203, 18, 305, 7, 262203, 18, 307, 7, 262203, 18, 310, 7, 262203, 20, 312, 7, 262203, 19, 325, 7, 262203, 18, 326, 7, 262203, 20, 327, 7, 262203, 18, 328, 7, 262203, 18, 330, 7, 262203, 19, 332, 7, 262203, 19, 333, 7, 262203, 19, 334, 7, 262203, 18, 335, 7, 262203, 20, 336, 7, 262203, 18, 349, 7, 262203, 18, 355, 7, 262203, 20, 356, 7, 262203, 18, 359, 7, 262203, 19, 364, 7, 262203, 124, 394, 7, 327745, 41, 225, 39, 40, 262205, 6, 226, 225, 196670, 224, 226, 327745, 41, 228, 39, 44, 262205, 6, 229, 228, 196670, 227, 229, 262205, 6, 230, 224, 327745, 52, 231, 49, 51, 262205, 6, 232, 231, 327854, 21, 233, 230, 232, 262312, 21, 234, 233, 196855, 236, 0, 262394, 234, 235, 236, 131320, 235, 262205, 6, 237, 227, 327745, 52, 238, 49, 191, 262205, 6, 239, 238, 327854, 21, 240, 237, 239, 131321, 236, 131320, 236, 458997, 21, 241, 233, 5, 240, 235, 196855, 243, 0, 262394, 241, 242, 243, 131320, 242, 65789, 131320, 243, 262205, 6, 246, 224, 262256, 10, 247, 246, 327809, 10, 249, 247, 248, 327745, 52, 250, 49, 51, 262205, 6, 251, 250, 262256, 10, 252, 251, 327816, 10, 253, 249, 252, 196670, 245, 253, 262205, 6, 255, 227, 262256, 10, 256, 255, 327809, 10, 257, 256, 248, 327745, 52, 258, 49, 191, 262205, 6, 259, 258, 262256, 10, 260, 259, 327816, 10, 261, 257, 260, 196670, 254, 261, 327745, 52, 263, 49, 51, 262205, 6, 264, 263, 262256, 10, 265, 264, 327745, 52, 266, 49, 191, 262205, 6, 267, 266, 262256, 10, 268, 267, 327816, 10, 269, 265, 268, 196670, 262, 269, 327745, 272, 273, 49, 271, 262205, 10, 274, 273, 393228, 10, 275, 1, 11, 274, 327813, 10, 276, 275, 248, 393228, 10, 277, 1, 15, 276, 196670, 270, 277, 262205, 10, 279, 270, 262205, 10, 280, 262, 327813, 10, 281, 279, 280, 393296, 14, 282, 281, 160, 160, 196670, 278, 282, 262205, 10, 284, 270, 393296, 14, 285, 160, 284, 160, 196670, 283, 285, 327745, 288, 289, 49, 287, 262205, 14, 290, 289, 262205, 10, 291, 245, 327811, 10, 292, 291, 248, 327813, 10, 293, 292, 92, 262205, 14, 294, 278, 327822, 14, 295, 294, 293, 327809, 14, 296, 290, 295, 262205, 10, 297, 254, 327811, 10, 298, 297, 248, 327813, 10, 299, 298, 92, 262205, 14, 300, 283, 327822, 14, 301, 300, 299, 327809, 14, 302, 296, 301, 393228, 14, 303, 1, 69, 302, 196670, 286, 303, 196670, 304, 208, 196670, 305, 306, 327745, 288, 308, 49, 120, 262205, 14, 309, 308, 196670, 307, 309, 262205, 14, 311, 286, 196670, 310, 311, 196670, 312, 40, 131321, 313, 131320, 313, 262390, 315, 316, 0, 131321, 317, 131320, 317, 262205, 6, 318, 312, 327745, 52, 320, 49, 319, 262205, 6, 321, 320, 327856, 21, 322, 318, 321, 262394, 322, 314, 315, 131320, 314, 262205, 14, 329, 307, 196670, 328, 329, 262205, 14, 331, 310, 196670, 330, 331, 196670, 332, 323, 196670, 333, 324, 720953, 21, 337, 30, 328, 330, 332, 333, 334, 335, 336, 262205, 10, 338, 334, 196670, 325, 338, 262205, 14, 339, 335, 196670, 326, 339, 262205, 6, 340, 336, 196670, 327, 340, 262312, 21, 341, 337, 196855, 343, 0, 262394, 341, 342, 343, 131320, 342, 262205, 14, 344, 305, 327813, 14, 345, 344, 208, 262205, 14, 346, 304, 327809, 14, 347, 346, 345, 196670, 304, 347, 131321, 315, 131320, 343, 262205, 14, 350, 307, 262205, 10, 351, 325, 262205, 14, 352, 310, 327822, 14, 353, 352, 351, 327809, 14, 354, 350, 353, 196670, 349, 354, 262205, 6, 357, 327, 196670, 356, 357, 327737, 14, 358, 34, 356, 196670, 355, 358, 262205, 14, 360, 326, 262201, 14, 361, 16, 327809, 14, 362, 360, 361, 393228, 14, 363, 1, 69, 362, 196670, 359, 363, 327745, 19, 365, 359, 40, 262205, 10, 366, 365, 327745, 19, 367, 359, 44, 262205, 10, 368, 367, 327745, 19, 369, 359, 218, 262205, 10, 370, 369, 458764, 10, 371, 1, 40, 368, 370, 458764, 10, 372, 1, 40, 366, 371, 196670, 364, 372, 262205, 6, 373, 312, 327852, 21, 374, 373, 218, 196855, 376, 0, 262394, 374, 375, 376, 131320, 375, 262201, 10, 377, 12, 262205, 10, 378, 364, 327866, 21, 379, 377, 378, 131321, 376, 131320, 376, 458997, 21, 380, 374, 343, 379, 375, 196855, 382, 0, 262394, 380, 381, 382, 131320, 381, 131321, 315, 131320, 382, 262205, 14, 384, 355, 262205, 10, 385, 364, 393296, 14, 386, 385, 385, 385, 327816, 14, 387, 384, 386, 262205, 14, 388, 305, 327813, 14, 389, 388, 387, 196670, 305, 389, 262205, 14, 390, 349, 196670, 307, 390, 262205, 14, 391, 359, 196670, 310, 391, 131321, 316, 131320, 316, 262205, 6, 392, 312, 327808, 6, 393, 392, 191, 196670, 312, 393, 131321, 313, 131320, 315, 262205, 14, 395, 304, 327761, 10, 396, 395, 0, 327761, 10, 397, 395, 1, 327761, 10, 398, 395, 2, 458832, 113, 399, 396, 397, 398, 94, 196670, 394, 399, 262205, 400, 403, 402, 262205, 6, 404, 224, 262268, 50, 405, 404, 262205, 6, 406, 227, 262268, 50, 407, 406, 327760, 408, 409, 405, 407, 262205, 113, 410, 394, 262243, 403, 409, 410, 65789, 65592, 327734, 6, 8, 0, 7, 131320, 9, 262203, 20, 36, 7, 262203, 20, 57, 7, 327745, 41, 42, 39, 40, 262205, 6, 43, 42, 327745, 41, 45, 39, 44, 262205, 6, 46, 45, 327745, 52, 53, 49, 51, 262205, 6, 54, 53, 327812, 6, 55, 46, 54, 327808, 6, 56, 43, 55, 196670, 36, 56, 262205, 6, 58, 36, 327812, 6, 60, 58, 59, 327745, 52, 62, 49, 61, 262205, 6, 63, 62, 327812, 6, 65, 63, 64, 327808, 6, 66, 60, 65, 196670, 57, 66, 262205, 6, 67, 57, 327812, 6, 69, 67, 68, 327808, 6, 71, 69, 70, 196670, 57, 71, 262205, 6, 72, 57, 131326, 72, 65592, 327734, 10, 12, 0, 11, 131320, 13, 262201, 6, 75, 8, 262256, 10, 76, 75, 327816, 10, 78, 76, 77, 131326, 78, 65592, 327734, 14, 16, 0, 15, 131320, 17, 262203, 18, 87, 7, 131321, 81, 131320, 81, 262390, 83, 84, 0, 131321, 85, 131320, 85, 262394, 86, 82, 83, 131320, 82, 262201, 10, 88, 12, 262201, 10, 89, 12, 262201, 10, 90, 12, 393296, 14, 91, 88, 89, 90, 327822, 14, 93, 91, 92, 393296, 14, 95, 94, 94, 94, 327811, 14, 96, 93, 95, 196670, 87, 96, 262205, 14, 97, 87, 262205, 14, 98, 87, 327828, 10, 99, 97, 98, 327864, 21, 100, 99, 94, 196855, 102, 0, 262394, 100, 101, 102, 131320, 101, 262205, 14, 103, 87, 131326, 103, 131320, 102, 131321, 84, 131320, 84, 131321, 81, 131320, 83, 196609, 14, 105, 131326, 105, 65592, 327734, 21, 30, 0, 22, 196663, 18, 23, 196663, 18, 24, 196663, 19, 25, 196663, 19, 26, 196663, 19, 27, 196663, 18, 28, 196663, 20, 29, 131320, 31, 262203, 20, 106, 7, 262203, 124, 125, 7, 262203, 18, 130, 7, 262203, 19, 133, 7, 262203, 18, 137, 7, 262203, 19, 141, 7, 262203, 19, 145, 7, 262203, 19, 153, 7, 262203, 19, 164, 7, 196670, 106, 40, 131321, 107, 131320, 107, 262390, 109, 110, 0, 131321, 111, 131320, 111, 262205, 6, 112, 106, 327748, 6, 118, 117, 0, 262268, 50, 119, 118, 327815, 50, 121, 119, 120, 262268, 6, 122, 121, 327856, 21, 123, 112, 122, 262394, 123, 108, 109, 131320, 108, 262205, 6, 126, 106, 393281, 127, 128, 117, 51, 126, 262205, 113, 129, 128, 196670, 125, 129, 262205, 113, 131, 125, 524367, 14, 132, 131, 131, 0, 1, 2, 196670, 130, 132, 327745, 19, 135, 125, 134, 262205, 10, 136, 135, 196670, 133, 136, 262205, 14, 138, 23, 262205, 14, 139, 130, 327811, 14, 140, 138, 139, 196670, 137, 140, 262205, 14, 142, 137, 262205, 14, 143, 24, 327828, 10, 144, 142, 143, 196670, 141, 144, 262205, 14, 146, 137, 262205, 14, 147, 137, 327828, 10, 148, 146, 147, 262205, 10, 149, 133, 262205, 10, 150, 133, 327813, 10, 151, 149, 150, 327811, 10, 152, 148, 151, 196670, 145, 152, 262205, 10, 154, 141, 262205, 10, 155, 141, 327813, 10, 156, 154, 155, 262205, 10, 157, 145, 327811, 10, 158, 156, 157, 196670, 153, 158, 262205, 10, 159, 153, 327866, 21, 161, 159, 160, 196855, 163, 0, 262394, 161, 162, 163, 131320, 162, 262205, 10, 165, 141, 262271, 10, 166, 165, 262205, 10, 167, 153, 393228, 10, 168, 1, 31, 167, 327811, 10, 169, 166, 168, 196670, 164, 169, 262205, 10, 170, 164, 262205, 10, 171, 25, 327866, 21, 172, 170, 171, 262205, 10, 173, 164, 262205, 10, 174, 26, 327864, 21, 175, 173, 174, 327847, 21, 176, 172, 175, 196855, 178, 0, 262394, 176, 177, 178, 131320, 177, 262205, 10, 179, 164, 196670, 27, 179, 262205, 14, 180, 23, 262205, 10, 181, 27, 262205, 14, 182, 24, 327822, 14, 183, 182, 181, 327809, 14, 184, 180, 183, 262205, 14, 185, 130, 327811, 14, 186, 184, 185, 393228, 14, 187, 1, 69, 186, 196670, 28, 187, 262205, 6, 188, 106, 196670, 29, 188, 131326, 86, 131320, 178, 131321, 163, 131320, 163, 131321, 110, 131320, 110, 262205, 6, 190, 106, 327808, 6, 192, 190, 191, 196670, 106, 192, 131321, 107, 131320, 109, 131326, 193, 65592, 327734, 14, 34, 0, 32, 196663, 20, 33, 131320, 35, 262203, 124, 210, 7, 262205, 6, 196, 33, 327748, 6, 201, 200, 0, 262268, 50, 202, 201, 327815, 50, 203, 202, 120, 262268, 6, 204, 203, 327854, 21, 205, 196, 204, 196855, 207, 0, 262394, 205, 206, 207, 131320, 206, 131326, 208, 131320, 207, 262205, 6, 211, 33, 393281, 127, 212, 200, 51, 211, 262205, 113, 213, 212, 196670, 210, 213, 327745, 19, 214, 210, 40, 262205, 10, 215, 214, 327745, 19, 216, 210, 44, 262205, 10, 217, 216, 327745, 19, 219, 210, 218, 262205, 10, 220, 219, 393296, 14, 221, 215, 217, 220, 131326, 221, 65592
};
    return computeShaderCode;
}

MultiGPUManager::MultiGPUManager() 
    : vulkanContext(nullptr), initialized(false) {}

MultiGPUManager::~MultiGPUManager() {
    cleanup();
}

uint32_t MultiGPUManager::findMemoryType(GPURenderResources& resources, uint32_t typeFilter, 
                                         VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

bool MultiGPUManager::createUniformBuffer(GPURenderResources& resources, uint32_t imageWidth, uint32_t imageHeight) {
    VkDevice device = resources.deviceInfo.device;
    
    VkDeviceSize bufferSize = sizeof(UniformData);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &resources.uniformBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create uniform buffer!" << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, resources.uniformBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(resources, memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.uniformBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate uniform buffer memory!" << std::endl;
        return false;
    }
    
    vkBindBufferMemory(device, resources.uniformBuffer, resources.uniformBufferMemory, 0);
    
    // Map memory for CPU access
    vkMapMemory(device, resources.uniformBufferMemory, 0, bufferSize, 0, &resources.mappedUniform);
    
    return true;
}

bool MultiGPUManager::createOutputImage(GPURenderResources& resources, uint32_t width, uint32_t height) {
    VkDevice device = resources.deviceInfo.device;
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &resources.outputImage) != VK_SUCCESS) {
        std::cerr << "Failed to create output image!" << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, resources.outputImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(resources, memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.outputImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        return false;
    }
    
    vkBindImageMemory(device, resources.outputImage, resources.outputImageMemory, 0);
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = resources.outputImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &resources.outputImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create image view!" << std::endl;
        return false;
    }
    
    return true;
}

bool MultiGPUManager::initialize(VulkanContext& context, const std::vector<uint32_t>& deviceIndices) {
    vulkanContext = &context;
    
    const auto& availableDevices = context.getDevices();
    
    // Lokale Kopie erstellen für die Geräteauswahl
    std::vector<uint32_t> selectedIndices = deviceIndices;
    
    if (selectedIndices.empty()) {
        std::cout << "Using all available GPUs..." << std::endl;
        for (uint32_t i = 0; i < availableDevices.size(); ++i) {
            selectedIndices.push_back(i);
        }
    }
    
    // Geräte initialisieren
    for (uint32_t index : selectedIndices) {
        if (index >= availableDevices.size()) {
            std::cerr << "Device index " << index << " out of range! Available devices: " 
                      << availableDevices.size() << std::endl;
            continue;
        }
        
        DeviceInfo info = availableDevices[index];
        
        // Device erstellen - dies modifiziert 'info' und setzt info.device
        if (!context.createDevice(info)) {
            std::cerr << "Failed to create device for GPU " << index << std::endl;
            continue;
        }
        
        GPURenderResources resources;
        // Verwende das aktualisierte 'info' mit dem erstellten Device, nicht die Kopie aus dem Context!
        resources.deviceInfo = info;
        
        // Command Pool erstellen
        resources.commandPool = context.createCommandPool(
            resources.deviceInfo.queueFamilyIndex, 
            resources.deviceInfo.device);
        
        // Fence erstellen
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled
        if (vkCreateFence(resources.deviceInfo.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) {
            std::cerr << "Failed to create fence!" << std::endl;
            continue;
        }
        
        gpuResources.push_back(std::move(resources));
        
        printf("Initialized GPU: %s (VRAM: %u MB)\n", 
               resources.deviceInfo.name.c_str(), 
               (unsigned int)(resources.deviceInfo.memorySize / (1024 * 1024)));
    }
    
    if (gpuResources.empty()) {
        std::cerr << "No GPUs could be initialized!" << std::endl;
        return false;
    }
    
    initialized = true;
    return true;
}

void MultiGPUManager::cleanup() {
    for (auto& resources : gpuResources) {
        VkDevice device = resources.deviceInfo.device;
        
        // Resources freigeben
        if (resources.fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, resources.fence, nullptr);
        }
        
        if (resources.computePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, resources.computePipeline, nullptr);
        }
        
        if (resources.pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, resources.pipelineLayout, nullptr);
        }
        
        if (resources.descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, resources.descriptorSetLayout, nullptr);
        }
        
        if (resources.descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, resources.descriptorPool, nullptr);
        }
        
        if (resources.uniformBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.uniformBuffer, nullptr);
        }
        if (resources.uniformBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.uniformBufferMemory, nullptr);
        }
        
        if (resources.sphereBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.sphereBuffer, nullptr);
        }
        if (resources.sphereBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.sphereBufferMemory, nullptr);
        }
        
        if (resources.materialBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.materialBuffer, nullptr);
        }
        if (resources.materialBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.materialBufferMemory, nullptr);
        }
        
        if (resources.outputImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, resources.outputImageView, nullptr);
        }
        if (resources.outputImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, resources.outputImage, nullptr);
        }
        if (resources.outputImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.outputImageMemory, nullptr);
        }
        
        if (resources.commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, resources.commandPool, nullptr);
        }
    }
    
    gpuResources.clear();
    vulkanContext = nullptr;
    initialized = false;
}

bool MultiGPUManager::setupScene(const std::vector<Sphere>& spheres, 
                                const std::vector<Material>& materials,
                                bool distributeAcrossGPUs) {
    if (!initialized) {
        std::cerr << "MultiGPUManager not initialized!" << std::endl;
        return false;
    }
    
    // Create output images and uniform buffers for all GPUs first (needed before descriptor set creation)
    // Note: Actual image size will be set in renderFrame, using placeholder here
    for (auto& resources : gpuResources) {
        // Output image will be created in renderFrame with correct dimensions
        resources.outputImage = VK_NULL_HANDLE;
        resources.uniformBuffer = VK_NULL_HANDLE;
    }
    
    if (distributeAcrossGPUs && gpuResources.size() > 1) {
        distributeSceneData(spheres, materials);
    } else {
        // Alle Daten auf jede GPU kopieren (einfacher, aber mehr VRAM)
        for (auto& resources : gpuResources) {
            VkDevice device = resources.deviceInfo.device;
            
            // Sphere Buffer
            VkDeviceSize sphereBufferSize = spheres.size() * sizeof(Sphere);
            if (sphereBufferSize > 0) {
                resources.sphereBuffer = vulkanContext->createBuffer(
                    sphereBufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    device);
                
                void* data;
                vkMapMemory(device, resources.sphereBufferMemory, 0, sphereBufferSize, 0, &data);
                memcpy(data, spheres.data(), sphereBufferSize);
                vkUnmapMemory(device, resources.sphereBufferMemory);
            }
            
            // Material Buffer
            VkDeviceSize materialBufferSize = materials.size() * sizeof(Material);
            if (materialBufferSize > 0) {
                resources.materialBuffer = vulkanContext->createBuffer(
                    materialBufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    device);
                
                void* data;
                vkMapMemory(device, resources.materialBufferMemory, 0, materialBufferSize, 0, &data);
                memcpy(data, materials.data(), materialBufferSize);
                vkUnmapMemory(device, resources.materialBufferMemory);
            }
        }
    }
    
    std::cout << "Scene setup complete on " << gpuResources.size() << " GPU(s)" << std::endl;
    return true;
}

void MultiGPUManager::distributeSceneData(const std::vector<Sphere>& spheres,
                                         const std::vector<Material>& materials) {
    std::cout << "Distributing scene data across " << gpuResources.size() << " GPUs..." << std::endl;
    
    size_t totalSpheres = spheres.size();
    size_t spheresPerGPU = (totalSpheres + gpuResources.size() - 1) / gpuResources.size();
    
    for (size_t gpuIdx = 0; gpuIdx < gpuResources.size(); ++gpuIdx) {
        auto& resources = gpuResources[gpuIdx];
        VkDevice device = resources.deviceInfo.device;
        
        // Berechnen welcher Bereich dieser GPU zugewiesen wird
        size_t startIdx = gpuIdx * spheresPerGPU;
        size_t endIdx = std::min(startIdx + spheresPerGPU, totalSpheres);
        size_t localSphereCount = endIdx - startIdx;
        
        std::cout << "  GPU " << gpuIdx << ": Spheres [" << startIdx << " - " << endIdx << "]" << std::endl;
        
        // Subset der Sphären für diese GPU
        std::vector<Sphere> localSpheres;
        if (localSphereCount > 0 && startIdx < totalSpheres) {
            localSpheres.assign(spheres.begin() + startIdx, spheres.begin() + endIdx);
        }
        
        // Sphere Buffer erstellen
        VkDeviceSize sphereBufferSize = localSpheres.size() * sizeof(Sphere);
        if (sphereBufferSize > 0) {
            resources.sphereBuffer = vulkanContext->createBuffer(
                sphereBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                device);
            
            // Memory allozieren und binden (vereinfacht)
            // In Produktion: findMemoryType mit proper memory type index
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, resources.sphereBuffer, &memReq);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            
            // Host-visible memory finden
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProps);
            
            uint32_t memoryTypeIndex = 0;
            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                if ((memReq.memoryTypeBits & (1 << i)) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    memoryTypeIndex = i;
                    break;
                }
            }
            allocInfo.memoryTypeIndex = memoryTypeIndex;
            
            if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.sphereBufferMemory) == VK_SUCCESS) {
                vkBindBufferMemory(device, resources.sphereBuffer, resources.sphereBufferMemory, 0);
                
                void* data;
                vkMapMemory(device, resources.sphereBufferMemory, 0, sphereBufferSize, 0, &data);
                memcpy(data, localSpheres.data(), sphereBufferSize);
                vkUnmapMemory(device, resources.sphereBufferMemory);
            }
        }
        
        // Materialien werden auf alle GPUs kopiert (da Referenzen von allen Sphären genutzt werden)
        VkDeviceSize materialBufferSize = materials.size() * sizeof(Material);
        if (materialBufferSize > 0) {
            resources.materialBuffer = vulkanContext->createBuffer(
                materialBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                device);
            
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, resources.materialBuffer, &memReq);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProps);
            
            uint32_t memoryTypeIndex = 0;
            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                if ((memReq.memoryTypeBits & (1 << i)) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    memoryTypeIndex = i;
                    break;
                }
            }
            allocInfo.memoryTypeIndex = memoryTypeIndex;
            
            if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.materialBufferMemory) == VK_SUCCESS) {
                vkBindBufferMemory(device, resources.materialBuffer, resources.materialBufferMemory, 0);
                
                void* data;
                vkMapMemory(device, resources.materialBufferMemory, 0, materialBufferSize, 0, &data);
                memcpy(data, materials.data(), materialBufferSize);
                vkUnmapMemory(device, resources.materialBufferMemory);
            }
        }
    }
}

void MultiGPUManager::assignTiles(uint32_t imageWidth, uint32_t imageHeight, uint32_t tileSize) {
    if (!initialized || gpuResources.empty()) {
        return;
    }
    
    // Tiles berechnen
    uint32_t tilesX = (imageWidth + tileSize - 1) / tileSize;
    uint32_t tilesY = (imageHeight + tileSize - 1) / tileSize;
    uint32_t totalTiles = tilesX * tilesY;
    
    std::cout << "Assigning " << totalTiles << " tiles (" << tilesX << "x" << tilesY 
              << ") to " << gpuResources.size() << " GPUs..." << std::endl;
    
    // Clear previous assignments
    for (auto& resources : gpuResources) {
        resources.assignedTiles.clear();
    }
    
    // Round-Robin Zuweisung der Tiles an GPUs
    uint32_t tileIndex = 0;
    for (uint32_t y = 0; y < tilesY; ++y) {
        for (uint32_t x = 0; x < tilesX; ++x) {
            uint32_t gpuIdx = tileIndex % gpuResources.size();
            
            RenderTile tile;
            tile.x = x * tileSize;
            tile.y = y * tileSize;
            tile.width = std::min(tileSize, imageWidth - tile.x);
            tile.height = std::min(tileSize, imageHeight - tile.y);
            tile.gpuIndex = gpuIdx;
            tile.completed = false;
            
            gpuResources[gpuIdx].assignedTiles.push_back(tile);
            
            tileIndex++;
        }
    }
    
    // Statistik
    for (size_t i = 0; i < gpuResources.size(); ++i) {
        std::cout << "  GPU " << i << ": " << gpuResources[i].assignedTiles.size() << " tiles" << std::endl;
    }
}

void MultiGPUManager::renderFrame(uint32_t imageWidth, uint32_t imageHeight, int sampleCount) {
    if (!initialized) {
        return;
    }
    
    std::cout << "Rendering frame (" << imageWidth << "x" << imageHeight 
              << ", " << sampleCount << " samples) on " << gpuResources.size() << " GPUs..." << std::endl;
    
    // Shader Code kompilieren (SPIR-V aus GLSL Source)
    std::vector<uint32_t> shaderCode;
    const char* shaderSource = getComputeShaderSource();
    
    // In einer echten Implementierung würde man hier den Shader zur Compile-Zeit kompilieren
    // Für diese Demo verwenden wir einen Placeholder - in der Praxis: glslc compiler aufrufen
    // Hier simulieren wir erfolgreiches Laden für die Demo
    std::cout << "  Note: Using embedded compute shader source (SPIR-V compilation would happen at build time)" << std::endl;
    
    // Für jede GPU den Compute Shader ausführen
    for (auto& resources : gpuResources) {
        if (resources.assignedTiles.empty()) {
            continue;
        }
        
        // Pipeline und DescriptorSet initialisieren falls noch nicht geschehen
        if (resources.pipelineLayout == VK_NULL_HANDLE || resources.descriptorSet == VK_NULL_HANDLE) {
            std::cout << "  Initializing compute pipeline for GPU " << resources.deviceInfo.deviceId << std::endl;
            
            // Ensure output image and uniform buffer exist
            if (resources.outputImage == VK_NULL_HANDLE) {
                if (!createOutputImage(resources, imageWidth, imageHeight)) {
                    std::cerr << "Failed to create output image for GPU " << resources.deviceInfo.deviceId << std::endl;
                    continue;
                }
            }
            
            if (resources.uniformBuffer == VK_NULL_HANDLE) {
                if (!createUniformBuffer(resources, imageWidth, imageHeight)) {
                    std::cerr << "Failed to create uniform buffer for GPU " << resources.deviceInfo.deviceId << std::endl;
                    continue;
                }
            }
            
            // Create descriptor set layout and pool
            if (!createDescriptorSet(resources)) {
                std::cerr << "Failed to create descriptor set for GPU " << resources.deviceInfo.deviceId << std::endl;
                continue;
            }
            
            // Create compute pipeline with compiled SPIR-V shader code
            const std::vector<uint32_t>& shaderCode = getCompiledShaderCode();
            if (shaderCode.empty()) {
                std::cerr << "ERROR: Shader code is empty! Cannot create compute pipeline." << std::endl;
                continue;
            }
            if (!createComputePipeline(resources, shaderCode)) {
                std::cerr << "Failed to create compute pipeline for GPU " << resources.deviceInfo.deviceId << std::endl;
                continue;
            }
        }
        
        VkDevice device = resources.deviceInfo.device;
        VkQueue queue = resources.deviceInfo.computeQueue;
        
        // Fence warten (vom vorherigen Frame)
        vkWaitForFences(device, 1, &resources.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &resources.fence);
        
        // Uniform Buffer aktualisieren
        if (resources.mappedUniform) {
            UniformData* uniformData = static_cast<UniformData*>(resources.mappedUniform);
            uniformData->width = imageWidth;
            uniformData->height = imageHeight;
            uniformData->maxDepth = 5;  // Max Ray Bounces
            uniformData->seed = static_cast<uint32_t>(sampleCount);
            uniformData->cameraOrigin[0] = 0.0f;
            uniformData->cameraOrigin[1] = 0.0f;
            uniformData->cameraOrigin[2] = -5.0f;
            uniformData->cameraDir[0] = 0.0f;
            uniformData->cameraDir[1] = 0.0f;
            uniformData->cameraDir[2] = 1.0f;
            uniformData->cameraUp[0] = 0.0f;
            uniformData->cameraUp[1] = 1.0f;
            uniformData->cameraUp[2] = 0.0f;
            uniformData->fov = 45.0f;
        }
        
        // Command Buffer erstellen und aufnehmen
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = resources.commandPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        // Bind Pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, resources.computePipeline);
        
        // Bind Descriptor Sets - with null check for safety
        if (resources.pipelineLayout != VK_NULL_HANDLE && resources.descriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                   resources.pipelineLayout, 0, 1, &resources.descriptorSet, 0, nullptr);
        } else {
            std::cerr << "Warning: pipelineLayout or descriptorSet is null for GPU " 
                      << resources.deviceInfo.deviceId << std::endl;
        }
        
        // Für jedes Tile einen Dispatch machen
        for (const auto& tile : resources.assignedTiles) {
            // Push Constants für Tile-Position (falls im Shader verwendet)
            // Oder Alternative: Die Tile-Informationen werden über die Work Group ID berechnet
            
            // Dispatch berechnen (16x16 Work Groups im Shader)
            uint32_t groupCountX = (tile.width + 15) / 16;
            uint32_t groupCountY = (tile.height + 15) / 16;
            
            vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
        }
        
        vkEndCommandBuffer(commandBuffer);
        
        // Submit Command Buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        if (vkQueueSubmit(queue, 1, &submitInfo, resources.fence) != VK_SUCCESS) {
            std::cerr << "Failed to submit command buffer!" << std::endl;
        }
        
        std::cout << "  GPU " << resources.deviceInfo.deviceId 
                  << " processing " << resources.assignedTiles.size() << " tiles" << std::endl;
        
        // Command Buffer freigeben
        vkFreeCommandBuffers(device, resources.commandPool, 1, &commandBuffer);
    }
}

void MultiGPUManager::gatherResults(void* outputBuffer, size_t bufferSize) {
    if (!initialized) {
        return;
    }
    
    // Ergebnisse von allen GPUs sammeln
    // Jedes GPU hat seinen eigenen Output-Image mit den Tile-Ergebnissen
    // Diese müssen an die richtige Position im Gesamtbuffer kopiert werden
    
    std::cout << "Gathering results from " << gpuResources.size() << " GPUs..." << std::endl;
    
    for (auto& resources : gpuResources) {
        if (!resources.assignedTiles.empty()) {
            // Für jedes Tile die Daten vom GPU-Image in den Hauptbuffer kopieren
            // In einer vollständigen Implementierung würde man hier:
            // 1. Image Layout Transition durchführen
            // 2. Copy Command von Image zu Buffer ausführen
            // 3. Daten an die richtige Position im outputBuffer kopieren
            
            for (const auto& tile : resources.assignedTiles) {
                // Hier würde der eigentliche Copy erfolgen
                // Die Tile-Informationen (x, y, width, height) sind verfügbar
                
                std::cout << "  Gathering tile at (" << tile.x << ", " << tile.y 
                          << ") size " << tile.width << "x" << tile.height
                          << " from GPU " << resources.deviceInfo.deviceId << std::endl;
            }
        }
    }
}

void MultiGPUManager::waitForAllGPUs() {
    if (!initialized) {
        return;
    }
    
    for (auto& resources : gpuResources) {
        VkDevice device = resources.deviceInfo.device;
        vkWaitForFences(device, 1, &resources.fence, VK_TRUE, UINT64_MAX);
    }
}

bool MultiGPUManager::createComputePipeline(GPURenderResources& resources, const std::vector<uint32_t>& shaderCode) {
    VkDevice device = resources.deviceInfo.device;
    
    // Shader Module erstellen
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
    createInfo.pCode = shaderCode.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module!" << std::endl;
        return false;
    }
    
    // Pipeline Layout erstellen
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &resources.descriptorSetLayout;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &resources.pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout!" << std::endl;
        vkDestroyShaderModule(device, shaderModule, nullptr);
        return false;
    }
    
    // Compute Pipeline erstellen
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = resources.pipelineLayout;
    
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resources.computePipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create compute pipeline!" << std::endl;
        vkDestroyShaderModule(device, shaderModule, nullptr);
        return false;
    }
    
    vkDestroyShaderModule(device, shaderModule, nullptr);
    return true;
}

bool MultiGPUManager::createDescriptorSet(GPURenderResources& resources) {
    VkDevice device = resources.deviceInfo.device;
    
    // Descriptor Set Layout erstellen
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    
    // Binding 0: Uniform Buffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 1: Sphere Storage Buffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 2: Material Storage Buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 3: Output Image (Storage Image)
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &resources.descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }
    
    // Descriptor Pool erstellen
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 2;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[3].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &resources.descriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
        return false;
    }
    
    // Descriptor Set allokieren
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = resources.descriptorPool;
    allocInfo.pSetLayouts = &resources.descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;
    
    if (vkAllocateDescriptorSets(device, &allocInfo, &resources.descriptorSet) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
        return false;
    }
    
    // Descriptor Set aktualisieren
    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    
    // Uniform Buffer
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = resources.uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformData);
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = resources.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
    
    // Sphere Buffer
    VkDescriptorBufferInfo sphereBufferInfo{};
    sphereBufferInfo.buffer = resources.sphereBuffer;
    sphereBufferInfo.offset = 0;
    sphereBufferInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = resources.descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &sphereBufferInfo;
    
    // Material Buffer
    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.buffer = resources.materialBuffer;
    materialBufferInfo.offset = 0;
    materialBufferInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = resources.descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &materialBufferInfo;
    
    // Output Image
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = resources.outputImageView;
    
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = resources.descriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    return true;
}

} // namespace vkpt
