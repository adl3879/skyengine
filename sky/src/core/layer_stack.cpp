#include "layer_stack.h"

namespace sky
{
LayerStack::~LayerStack() {}

void LayerStack::pushLayer(Layer *layer)
{
    layers_.emplace(layers_.begin() + layerInsertIndex_, layer);
    layerInsertIndex_++;
}

void LayerStack::pushOverlay(Layer *overlay) { layers_.emplace_back(overlay); }

void LayerStack::popLayer(Layer *layer)
{
    auto it = std::find(layers_.begin(), layers_.begin() + layerInsertIndex_, layer);
    if (it != layers_.begin() + layerInsertIndex_)
    {
        layer->onDetach();
        layers_.erase(it);
        layerInsertIndex_--;
    }
}

void LayerStack::popOverlay(Layer *overlay)
{
    auto it = std::find(layers_.begin() + layerInsertIndex_, layers_.end(), overlay);
    if (it != layers_.end())
    {
        overlay->onDetach();
        layers_.erase(it);
    }
}
}