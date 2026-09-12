/********************************************************************************
 * MIT License
 *
 * Copyright (c) 2025-2026 kuloPo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#include "ApiTableModel.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <unordered_map>

#include "JsonWrapper.hpp"
#include "common.hpp"

namespace {

const std::unordered_map<std::string_view, std::vector<std::string_view>> kApiArgNames = {
    {"vkCreateInstance", {"/pCreateInfo/pApplicationInfo/pApplicationName", "/pCreateInfo/enabledExtensionCount", "/pCreateInfo/ppEnabledExtensionNames", "/pInstance"}},
    {"vkEnumeratePhysicalDevices", {"/pPhysicalDeviceCount", "/pPhysicalDevices"}},
    {"vkGetPhysicalDeviceProperties", {"/physicalDevice", "/pProperties/deviceName"}},
    {"vkGetPhysicalDeviceFeatures", {"/physicalDevice", "/pFeatures"}},
    {"vkGetPhysicalDeviceFormatProperties", {"/physicalDevice", "/format"}},
    {"vkGetPhysicalDeviceMemoryProperties", {"/physicalDevice", "/pMemoryProperties"}},
    {"vkGetPhysicalDeviceImageFormatProperties", {"/physicalDevice", "/format", "/type", "/tiling", "/usage"}},
    {"vkGetPhysicalDeviceSparseImageFormatProperties", {"/physicalDevice", "/format", "/type", "/usage"}},
    {"vkGetPhysicalDeviceQueueFamilyProperties", {"/physicalDevice", "/pQueueFamilyPropertyCount"}},
    {"vkCreateDevice", {"/physicalDevice", "/pCreateInfo/enabledExtensionCount", "/pCreateInfo/ppEnabledExtensionNames", "/pDevice"}},
    {"vkGetDeviceQueue", {"/queueFamilyIndex", "/queueIndex", "/pQueue"}},
    {"vkCreateFence", {"/pCreateInfo/flags", "/pFence"}},
    {"vkResetFences", {"/fenceCount", "/pFences"}},
    {"vkCreateEvent", {"/pCreateInfo/flags", "/pEvent"}},
    {"vkCreateSemaphore", {"/pCreateInfo/flags", "/pSemaphore"}},
    {"vkCreateQueryPool", {"/pCreateInfo/queryType", "/pCreateInfo/queryCount", "/pQueryPool"}},
    {"vkCreateBuffer", {"/pCreateInfo/size", "/pCreateInfo/usage", "/pBuffer"}},
    {"vkCreateBufferView", {"/pCreateInfo/buffer", "/pCreateInfo/format", "/pView"}},
    {"vkCreateImage", {"/pCreateInfo/extent/width", "/pCreateInfo/extent/height", "/pCreateInfo/format", "/pCreateInfo/usage", "/pImage"}},
    {"vkCreateImageView", {"/pCreateInfo/image", "/pCreateInfo/viewType", "/pCreateInfo/format", "/pView"}},
    {"vkCreateRenderPass", {"/pCreateInfo/attachmentCount", "/pCreateInfo/subpassCount", "/pRenderPass"}},
    {"vkCreateFramebuffer", {"/pCreateInfo/renderPass", "/pCreateInfo/width", "/pCreateInfo/height", "/pFramebuffer"}},
    {"vkCreatePipelineCache", {"/pCreateInfo/initialDataSize", "/pPipelineCache"}},
    {"vkCreateDescriptorSetLayout", {"/pCreateInfo/bindingCount", "/pSetLayout"}},
    {"vkCreatePipelineLayout", {"/pCreateInfo/setLayoutCount", "/pPipelineLayout"}},
    {"vkCreateDescriptorPool", {"/pCreateInfo/maxSets", "/pDescriptorPool"}},
    {"vkCreateSampler", {"/pCreateInfo/magFilter", "/pCreateInfo/minFilter", "/pSampler"}},
    {"vkCreateShaderModule", {"/pCreateInfo/codeSize", "/pShaderModule"}},
    {"vkCreateGraphicsPipelines", {"/createInfoCount", "/pPipelines"}},
    {"vkCreateComputePipelines", {"/createInfoCount", "/pPipelines"}},
    {"vkCreateCommandPool", {"/pCreateInfo/queueFamilyIndex", "/pCreateInfo/flags", "/pCommandPool"}},
    {"vkAllocateMemory", {"/pAllocateInfo/allocationSize", "/pAllocateInfo/memoryTypeIndex", "/pMemory"}},
    {"vkAllocateCommandBuffers", {"/pAllocateInfo/commandPool", "/pAllocateInfo/level", "/pAllocateInfo/commandBufferCount", "/pCommandBuffers"}},
    {"vkAllocateDescriptorSets", {"/pAllocateInfo/descriptorPool", "/pAllocateInfo/descriptorSetCount", "/pDescriptorSets"}},
    {"vkBindBufferMemory", {"/buffer", "/memory", "/memoryOffset"}},
    {"vkBindImageMemory", {"/image", "/memory", "/memoryOffset"}},
    {"vkBeginCommandBuffer", {"/commandBuffer", "/pBeginInfo/flags"}},
    {"vkCmdCopyBuffer", {"/commandBuffer", "/srcBuffer", "/dstBuffer", "/regionCount"}},
    {"vkCmdCopyBufferToImage", {"/commandBuffer", "/srcBuffer", "/dstImage", "/dstImageLayout", "/regionCount"}},
    {"vkCmdCopyImage", {"/commandBuffer", "/srcImage", "/srcImageLayout", "/dstImage", "/dstImageLayout", "/regionCount"}},
    {"vkCmdCopyImageToBuffer", {"/commandBuffer", "/srcImage", "/srcImageLayout", "/dstBuffer", "/regionCount"}},
    {"vkCmdBlitImage", {"/commandBuffer", "/srcImage", "/srcImageLayout", "/dstImage", "/dstImageLayout", "/filter"}},
    {"vkCmdSetEvent", {"/commandBuffer", "/event", "/stageMask"}},
    {"vkCmdWaitEvents", {"/commandBuffer", "/eventCount", "/pEvents", "/srcStageMask", "/dstStageMask"}},
    {"vkCmdResetEvent", {"/commandBuffer", "/event", "/stageMask"}},
    {"vkCmdBeginRenderPass", {"/commandBuffer", "/pRenderPassBegin/renderPass", "/pRenderPassBegin/framebuffer", "/contents"}},
    {"vkCmdBindPipeline", {"/commandBuffer", "/pipelineBindPoint", "/pipeline"}},
    {"vkCmdBindVertexBuffers", {"/commandBuffer", "/firstBinding", "/bindingCount", "/pBuffers"}},
    {"vkCmdBindIndexBuffer", {"/commandBuffer", "/buffer", "/offset", "/indexType"}},
    {"vkCmdBindDescriptorSets", {"/commandBuffer", "/pipelineBindPoint", "/layout", "/firstSet", "/descriptorSetCount", "/pDescriptorSets"}},
    {"vkCmdSetViewport", {"/commandBuffer", "/firstViewport", "/viewportCount"}},
    {"vkCmdSetScissor", {"/commandBuffer", "/firstScissor", "/scissorCount"}},
    {"vkCmdSetLineWidth", {"/commandBuffer", "/lineWidth"}},
    {"vkCmdSetBlendConstants", {"/commandBuffer", "/blendConstants"}},
    {"vkCmdSetDepthBias", {"/commandBuffer", "/depthBiasConstantFactor", "/depthBiasClamp", "/depthBiasSlopeFactor"}},
    {"vkCmdSetDepthBounds", {"/commandBuffer", "/minDepthBounds", "/maxDepthBounds"}},
    {"vkCmdSetStencilCompareMask", {"/commandBuffer", "/faceMask", "/compareMask"}},
    {"vkCmdSetStencilReference", {"/commandBuffer", "/faceMask", "/reference"}},
    {"vkCmdSetStencilWriteMask", {"/commandBuffer", "/faceMask", "/writeMask"}},
    {"vkCmdPushConstants", {"/commandBuffer", "/layout", "/stageFlags", "/offset", "/size"}},
    {"vkCmdClearAttachments", {"/commandBuffer", "/attachmentCount", "/rectCount"}},
    {"vkCmdClearColorImage", {"/commandBuffer", "/image", "/imageLayout", "/rangeCount"}},
    {"vkCmdClearDepthStencilImage", {"/commandBuffer", "/image", "/imageLayout", "/rangeCount"}},
    {"vkCmdFillBuffer", {"/commandBuffer", "/dstBuffer", "/dstOffset", "/size"}},
    {"vkCmdUpdateBuffer", {"/commandBuffer", "/dstBuffer", "/dstOffset", "/dataSize"}},
    {"vkCmdBeginQuery", {"/commandBuffer", "/queryPool", "/query"}},
    {"vkCmdEndQuery", {"/commandBuffer", "/queryPool", "/query"}},
    {"vkCmdResetQueryPool", {"/commandBuffer", "/queryPool", "/firstQuery", "/queryCount"}},
    {"vkCmdCopyQueryPoolResults", {"/commandBuffer", "/queryPool", "/firstQuery", "/queryCount", "/dstBuffer", "/stride"}},
    {"vkCmdWriteTimestamp", {"/commandBuffer", "/pipelineStage", "/queryPool", "/query"}},
    {"vkCmdDispatch", {"/commandBuffer", "/groupCountX", "/groupCountY", "/groupCountZ"}},
    {"vkCmdDispatchIndirect", {"/commandBuffer", "/buffer", "/offset"}},
    {"vkCmdDraw", {"/commandBuffer", "/vertexCount", "/instanceCount", "/firstVertex", "/firstInstance"}},
    {"vkCmdDrawIndexed", {"/commandBuffer", "/indexCount", "/instanceCount", "/firstIndex", "/vertexOffset", "/firstInstance"}},
    {"vkCmdDrawIndirect", {"/commandBuffer", "/buffer", "/offset", "/drawCount"}},
    {"vkCmdDrawIndexedIndirect", {"/commandBuffer", "/buffer", "/offset", "/drawCount"}},
    {"vkCmdResolveImage", {"/commandBuffer", "/srcImage", "/srcImageLayout", "/dstImage", "/dstImageLayout", "/regionCount"}},
    {"vkCmdExecuteCommands", {"/commandBuffer", "/commandBufferCount", "/pCommandBuffers"}},
    {"vkCmdNextSubpass", {"/commandBuffer", "/contents"}},
    {"vkCmdPipelineBarrier", {"/commandBuffer", "/srcStageMask", "/dstStageMask", "/memoryBarrierCount", "/bufferMemoryBarrierCount", "/imageMemoryBarrierCount"}},
    {"vkCmdEndRenderPass", {"/commandBuffer"}},
    {"vkEndCommandBuffer", {"/commandBuffer"}},
    {"vkQueueSubmit", {"/submitCount", "/fence"}},
    {"vkQueueWaitIdle", {"/queue"}},
    {"vkQueueBindSparse", {"/bindInfoCount", "/fence"}},
    {"vkDeviceWaitIdle", {}},
    {"vkWaitForFences", {"/fenceCount", "/pFences", "/waitAll", "/timeout"}},
    {"vkGetFenceStatus", {"/fence"}},
    {"vkGetEventStatus", {"/event"}},
    {"vkSetEvent", {"/event"}},
    {"vkResetEvent", {"/event"}},
    {"vkGetQueryPoolResults", {"/queryPool", "/firstQuery", "/queryCount", "/stride"}},
    {"vkGetRenderAreaGranularity", {"/renderPass"}},
    {"vkGetPipelineCacheData", {"/pipelineCache", "/pDataSize"}},
    {"vkMergePipelineCaches", {"/dstCache", "/srcCacheCount"}},
    {"vkGetBufferMemoryRequirements", {"/buffer"}},
    {"vkGetImageMemoryRequirements", {"/image"}},
    {"vkGetImageSparseMemoryRequirements", {"/image", "/pSparseMemoryRequirementCount"}},
    {"vkGetImageSubresourceLayout", {"/image", "/pSubresource/mipLevel", "/pSubresource/arrayLayer"}},
    {"vkGetDeviceMemoryCommitment", {"/memory", "/pCommittedMemoryInBytes"}},
    {"vkMapMemory", {"/memory", "/offset", "/size"}},
    {"vkFlushMappedMemoryRanges", {"/memoryRangeCount"}},
    {"vkInvalidateMappedMemoryRanges", {"/memoryRangeCount"}},
    {"vkUnmapMemory", {"/memory"}},
    {"vkUpdateDescriptorSets", {"/descriptorWriteCount", "/descriptorCopyCount"}},
    {"vkResetCommandBuffer", {"/commandBuffer", "/flags"}},
    {"vkResetCommandPool", {"/commandPool", "/flags"}},
    {"vkFreeDescriptorSets", {"/descriptorPool", "/descriptorSetCount"}},
    {"vkResetDescriptorPool", {"/descriptorPool", "/flags"}},
    {"vkFreeCommandBuffers", {"/commandPool", "/commandBufferCount", "/pCommandBuffers"}},
    {"vkFreeMemory", {"/memory"}},
    {"vkDestroyShaderModule", {"/shaderModule"}},
    {"vkDestroyPipeline", {"/pipeline"}},
    {"vkDestroyPipelineLayout", {"/pipelineLayout"}},
    {"vkDestroyPipelineCache", {"/pipelineCache"}},
    {"vkDestroyDescriptorPool", {"/descriptorPool"}},
    {"vkDestroyDescriptorSetLayout", {"/descriptorSetLayout"}},
    {"vkDestroyFramebuffer", {"/framebuffer"}},
    {"vkDestroyRenderPass", {"/renderPass"}},
    {"vkDestroyImageView", {"/imageView"}},
    {"vkDestroyImage", {"/image"}},
    {"vkDestroySampler", {"/sampler"}},
    {"vkDestroyBufferView", {"/bufferView"}},
    {"vkDestroyBuffer", {"/buffer"}},
    {"vkDestroyQueryPool", {"/queryPool"}},
    {"vkDestroyCommandPool", {"/commandPool"}},
    {"vkDestroyEvent", {"/event"}},
    {"vkDestroyFence", {"/fence"}},
    {"vkDestroySemaphore", {"/semaphore"}},
    {"vkDestroyDevice", {"/device"}},
    {"vkDestroyInstance", {"/instance"}},
};

} // namespace

ApiTableModel::ApiTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

bool ApiTableModel::OnLoadFile(const QString& filePath) {
    std::string path = filePath.toStdString();

    auto error = simdjson::padded_string::load(path).get(m_Json);
    if (error) {
        LOGW("Failed to load \"%s\": %s.\n\n"
             "The file may have been moved, deleted, or is locked by another process.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    simdjson::ondemand::parser scanner;
    simdjson::ondemand::document doc;
    error = scanner.iterate(m_Json).get(doc);
    if (error) {
        LOGW("Failed to parse \"%s\": %s.\n\n"
             "The file may be corrupted or is not a valid GFXReconstruct capture.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    simdjson::ondemand::array arr;
    error = doc.get_array().get(arr);
    if (error) {
        LOGW("Unexpected JSON structure in \"%s\": %s.\n\n"
             "The file does not contain a valid GFXReconstruct capture array.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    size_t idx = 0;
    m_EntryRanges.clear();
    m_ApiEntries.clear();
    m_ApiEntries.push_back({});

    beginResetModel();

    for (auto elemResult : arr) {
        simdjson::ondemand::value elem;
        if (const auto e = elemResult.get(elem); e) {
            LOGW("Failed to parse \"%s\": %s.\n\n"
                 "The file may be corrupted or is not a valid GFXReconstruct capture.",
                 path.c_str(), simdjson::error_message(e));
            endResetModel();
            return false;
        }

        std::string_view raw;
        (void)elem.raw_json().get(raw);

        const size_t start = static_cast<size_t>(raw.data() - m_Json.data());
        const size_t len = raw.size();
        m_EntryRanges.push_back({start, len});

        simdjson::dom::element entry = JsonGetElement(m_EntryParser, m_Json, start, len);

        const simdjson::dom::object obj = JsonGetObject(entry);

        try {
            std::string_view name;
            if (!obj["function"]["name"].get(name)) {
                m_ApiEntries.back().push_back(idx);
                if (name == "vkQueuePresentKHR") {
                    m_ApiEntries.push_back({});
                }
            }
        } catch (const simdjson::simdjson_error&) {
        }
        idx++;
    }
    // A trailing vkQueuePresentKHR would have opened an empty frame; drop it.
    if (m_ApiEntries.size() > 1 && m_ApiEntries.back().empty()) {
        m_ApiEntries.pop_back();
    }

    m_Frame = 0;
    RefreshFilteredRow();
    endResetModel();
    return true;
}

void ApiTableModel::SetFrame(int frame) {
    if (frame == m_Frame) return;
    beginResetModel();
    m_Frame = frame;
    RefreshFilteredRow();
    endResetModel();
}

void ApiTableModel::SetFilter(const QString& filter) {
    if (filter == m_FilterString) return;
    m_FilterString = filter;
    beginResetModel();
    RefreshFilteredRow();
    endResetModel();
}

size_t ApiTableModel::RawEntryIndex(int row) const {
    return m_FilteredRowIndexes[row];
}

int ApiTableModel::FindRowByEntry(size_t entryIndex) const {
    auto it = std::lower_bound(m_FilteredRowIndexes.begin(), m_FilteredRowIndexes.end(), entryIndex);
    if (it != m_FilteredRowIndexes.end() && *it == entryIndex) {
        return static_cast<int>(it - m_FilteredRowIndexes.begin());
    }
    return -1;
}

simdjson::dom::object ApiTableModel::GetEntryObject(size_t entryIndex) const {
    const Range& range = m_EntryRanges[entryIndex];
    simdjson::dom::element entry = JsonGetElement(m_EntryParser, m_Json, range.start, range.len);
    return JsonGetObject(entry);
}

size_t ApiTableModel::GetFrameCount() const {
    return m_ApiEntries.size();
}

size_t ApiTableModel::GetFrameTotalAPICount() const {
    return m_ApiEntries.empty() ? 0 : m_ApiEntries[m_Frame].size();
}

void ApiTableModel::RefreshFilteredRow() {
    m_FilteredRowIndexes.clear();
    const auto& frame = m_ApiEntries[m_Frame];
    for (const size_t entryIndex : frame) {
        if (m_FilterString.isEmpty() || EntryNameMatches(entryIndex, m_FilterString)) {
            m_FilteredRowIndexes.push_back(entryIndex);
        }
    }
}

bool ApiTableModel::EntryNameMatches(size_t entryIndex, const QString& needle) const {
    const simdjson::dom::object obj = GetEntryObject(entryIndex);
    const auto name = JsonGetApiName(obj);
    return QAnyStringView(name).toString().contains(needle, Qt::CaseInsensitive);
}

int ApiTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_FilteredRowIndexes.size();
}

int ApiTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 4;
}

QVariant ApiTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
    switch (section) {
        case 0: return QStringLiteral("Index");
        case 1: return QStringLiteral("Return");
        case 2: return QStringLiteral("Name");
        case 3: return QStringLiteral("Args");
        default: {
            LOGE("ApiTableModel::headerData: unexpected section %d", section);
            return {};
        }
    }
}

QVariant ApiTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return {};

    simdjson::dom::object obj = GetEntryObject(m_FilteredRowIndexes[index.row()]);

    try {
        switch (index.column()) {
            case 0: return QString::number(JsonGetApiIndex(obj));
            case 1: return JsonGetApiReturn(obj);
            case 2: {
                const auto name = JsonGetApiName(obj);
                return QAnyStringView(name).toString();
            }
            case 3: {
                const auto it = kApiArgNames.find(JsonGetApiName(obj));
                if (it == kApiArgNames.end()) return QStringLiteral("()");

                const auto args = JsonGetApiArgs(obj);
                QString text;
                for (const std::string_view pointer : it->second) {
                    if (!text.isEmpty()) text += ", ";
                    text += QAnyStringView(pointer.substr(pointer.rfind('/') + 1)).toString() + "=" + JsonElementToString(args.at_pointer(pointer).value());
                }
                return QStringLiteral("(%1)").arg(text);
            }
            default: {
                LOGE("ApiTableModel::data: unexpected column %d", index.column());
                return {};
            }
        }
    } catch (const simdjson::simdjson_error&) {
        return {};
    }
}
