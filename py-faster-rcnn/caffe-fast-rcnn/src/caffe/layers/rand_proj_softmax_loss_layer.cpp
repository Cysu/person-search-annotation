#include <algorithm>
#include <functional>
#include <utility>
#include <cfloat>
#include <vector>

#include "caffe/layers/rand_proj_softmax_loss_layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/rng.hpp"

namespace caffe {

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::LayerSetUp(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  LossLayer<Dtype>::LayerSetUp(bottom, top);
  softmax_axis_ =
      bottom[0]->CanonicalAxisIndex(this->layer_param_.softmax_param().axis());
  rand_proj_num_ = this->layer_param_.loss_param().rand_proj_num();
  rand_proj_policy_ = this->layer_param_.loss_param().rand_proj_policy();
  if (rand_proj_policy_ != "topk" &&
      rand_proj_policy_ != "random") {
    LOG(FATAL) << "Cannot recognize the rand_proj_policy parameter";
  }

  LayerParameter softmax_param(this->layer_param_);
  softmax_param.set_type("Softmax");
  softmax_layer_ = LayerRegistry<Dtype>::CreateLayer(softmax_param);
  softmax_bottom_vec_.clear();
  softmax_bottom_vec_.push_back(&proj_);
  softmax_top_vec_.clear();
  softmax_top_vec_.push_back(&prob_);
  vector<int> proj_dims = bottom[0]->shape();
  proj_dims[softmax_axis_] = rand_proj_num_;
  proj_.Reshape(proj_dims);
  softmax_layer_->SetUp(softmax_bottom_vec_, softmax_top_vec_);

  has_ignore_label_ =
    this->layer_param_.loss_param().has_ignore_label();
  if (has_ignore_label_) {
    ignore_label_ = this->layer_param_.loss_param().ignore_label();
  }
  if (!this->layer_param_.loss_param().has_normalization() &&
      this->layer_param_.loss_param().has_normalize()) {
    normalization_ = this->layer_param_.loss_param().normalize() ?
                     LossParameter_NormalizationMode_VALID :
                     LossParameter_NormalizationMode_BATCH_SIZE;
  } else {
    normalization_ = this->layer_param_.loss_param().normalization();
  }
}

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::Reshape(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  LossLayer<Dtype>::Reshape(bottom, top);
  outer_num_ = bottom[0]->count(0, softmax_axis_);
  channels_ = bottom[0]->shape(softmax_axis_);
  inner_num_ = bottom[0]->count(softmax_axis_ + 1);
  CHECK_EQ(outer_num_ * inner_num_, bottom[1]->count())
      << "Number of labels must match number of predictions; "
      << "e.g., if softmax axis == 1 and prediction shape is (N, C, H, W), "
      << "label count (number of labels) must be N*H*W, "
      << "with integer values in {0, 1, ..., C-1}.";
  CHECK_GT(channels_, rand_proj_num_)
      << "Number of channels must be greater than that of random projection ";
  vector<int> proj_dims = bottom[0]->shape();
  proj_dims[softmax_axis_] = rand_proj_num_;
  proj_.Reshape(proj_dims);
  softmax_layer_->Reshape(softmax_bottom_vec_, softmax_top_vec_);
  rand_proj_index_.ReshapeLike(proj_);
  index_vec_.resize(channels_);
  for (int i = 0; i < channels_; ++i) {
    index_vec_[i] = i;
  }
}

template <typename Dtype>
Dtype RandProjSoftmaxWithLossLayer<Dtype>::get_normalizer(
    LossParameter_NormalizationMode normalization_mode, int valid_count) {
  Dtype normalizer;
  switch (normalization_mode) {
    case LossParameter_NormalizationMode_FULL:
      normalizer = Dtype(outer_num_ * inner_num_);
      break;
    case LossParameter_NormalizationMode_VALID:
      if (valid_count == -1) {
        normalizer = Dtype(outer_num_ * inner_num_);
      } else {
        normalizer = Dtype(valid_count);
      }
      break;
    case LossParameter_NormalizationMode_BATCH_SIZE:
      normalizer = Dtype(outer_num_);
      break;
    case LossParameter_NormalizationMode_NONE:
      normalizer = Dtype(1);
      break;
    default:
      LOG(FATAL) << "Unknown normalization mode: "
          << LossParameter_NormalizationMode_Name(normalization_mode);
  }
  // Some users will have no labels for some examples in order to 'turn off' a
  // particular loss in a multi-task setup. The max prevents NaNs in that case.
  return std::max(Dtype(1.0), normalizer);
}

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::fill_rand_proj_index(
    const vector<Blob<Dtype>*>& bottom) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* label = bottom[1]->cpu_data();
  Dtype* rand_proj_index_data = rand_proj_index_.mutable_cpu_data();
  const int dim = channels_ * inner_num_;
  const int dim_proj = rand_proj_num_ * inner_num_;
  for (int i = 0; i < outer_num_; ++i) {
    for (int j = 0; j < inner_num_; ++j) {
      const int label_value = static_cast<int>(label[i * inner_num_ + j]);
      if (has_ignore_label_ && label_value == ignore_label_) {
        // Just put 0 to rand_proj_num_ - 1
        for (int k = 0; k < rand_proj_num_; ++k) {
          rand_proj_index_data[i * dim_proj + k * inner_num_ + j] = (Dtype)k;
        }
      } else {
        DCHECK_GE(label_value, 0);
        DCHECK_LT(label_value, prob_.shape(softmax_axis_));
        // Here we hard coded two rules:
        //   1. Put the label_value to the first
        //   2. Put the channels_-1 (background class) to the second
        int k = 0;
        rand_proj_index_data[i * dim_proj + k * inner_num_ + j] = (Dtype)label_value;
        k++;
        const int bg_label = channels_ - 1;
        if (label_value != bg_label) {
          rand_proj_index_data[i * dim_proj + k * inner_num_ + j] = (Dtype)bg_label;
          k++;
        }
        if (rand_proj_policy_ == "random") {
          shuffle(index_vec_.begin(), index_vec_.end());
          for (int x = 0; x < index_vec_.size(); ++x) {
            if (k >= rand_proj_num_) break;
            const int index = index_vec_[x];
            if (index != label_value && index != bg_label) {
              rand_proj_index_data[i * dim_proj + k * inner_num_ + j] = (Dtype)index;
              k++;
            }
          }
        } else {
          vector<std::pair<Dtype, int> > score_index_vector;
          for (int c = 0; c < channels_; ++c) {
            score_index_vector.push_back(std::make_pair(
                bottom_data[i * dim + c * inner_num_ + j], c));
          }
          std::partial_sort(score_index_vector.begin(),
                            score_index_vector.begin() + rand_proj_num_,
                            score_index_vector.end(),
                            std::greater<std::pair<Dtype, int> >());
          for (int x = 0; x < rand_proj_num_; ++x) {
            if (k >= rand_proj_num_) break;
            const int index = score_index_vector[x].second;
            if (index != label_value && index != bg_label) {
              rand_proj_index_data[i * dim_proj + k * inner_num_ + j] = (Dtype)index;
              k++;
            }
          }
        }
      }
    }
  }
}

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::rand_proj(
    const vector<Blob<Dtype>*>& bottom) {
  fill_rand_proj_index(bottom);
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* rand_proj_index_data = rand_proj_index_.cpu_data();
  Dtype* proj_data = proj_.mutable_cpu_data();
  const int dim = channels_ * inner_num_;
  int index = 0;
  for (int i = 0; i < outer_num_; ++i) {
    for (int k = 0; k < rand_proj_num_; ++k) {
      for (int j = 0; j < inner_num_; ++j) {
        const int c = static_cast<int>(rand_proj_index_data[index]);
        proj_data[index++] = bottom_data[i * dim + c * inner_num_ + j];
      }
    }
  }
}

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::Forward_cpu(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  rand_proj(bottom);
  // The forward pass computes the softmax prob values.
  softmax_layer_->Forward(softmax_bottom_vec_, softmax_top_vec_);
  const Dtype* prob_data = prob_.cpu_data();
  const Dtype* label = bottom[1]->cpu_data();
  int dim = prob_.count() / outer_num_;
  int count = 0;
  Dtype loss = 0;
  for (int i = 0; i < outer_num_; ++i) {
    for (int j = 0; j < inner_num_; j++) {
      const int label_value = static_cast<int>(label[i * inner_num_ + j]);
      if (has_ignore_label_ && label_value == ignore_label_) {
        continue;
      }
      // Here we hard coded that when fill up the random projection index,
      // we always put the target label to the first element.
      const int proj_index = 0;
      loss -= log(std::max(prob_data[i * dim + proj_index * inner_num_ + j],
                           Dtype(FLT_MIN)));
      ++count;
    }
  }
  top[0]->mutable_cpu_data()[0] = loss / get_normalizer(normalization_, count);
}

template <typename Dtype>
void RandProjSoftmaxWithLossLayer<Dtype>::Backward_cpu(
    const vector<Blob<Dtype>*>& top, const vector<bool>& propagate_down,
    const vector<Blob<Dtype>*>& bottom) {
  if (propagate_down[1]) {
    LOG(FATAL) << this->type()
               << " Layer cannot backpropagate to label inputs.";
  }
  if (propagate_down[0]) {
    Dtype* bottom_diff = bottom[0]->mutable_cpu_diff();
    caffe_set(bottom[0]->count(), (Dtype)0, bottom_diff);
    const Dtype* prob_data = prob_.cpu_data();
    const Dtype* label = bottom[1]->cpu_data();
    const Dtype* rand_proj_index_data = rand_proj_index_.cpu_data();
    int dim = channels_ * inner_num_;
    int dim_proj = rand_proj_num_ * inner_num_;
    int count = 0;
    for (int i = 0; i < outer_num_; ++i) {
      for (int j = 0; j < inner_num_; ++j) {
        const int label_value = static_cast<int>(label[i * inner_num_ + j]);
        if (has_ignore_label_ && label_value == ignore_label_) {
          continue;
        }
        // Project back the gradients
        for (int k = 0; k < rand_proj_num_; ++k) {
          const int index = i * dim_proj + k * inner_num_ + j;
          const int c = static_cast<int>(rand_proj_index_data[index]);
          bottom_diff[i * dim + c * inner_num_ + j] = prob_data[index];
        }
        bottom_diff[i * dim + label_value * inner_num_ + j] -= 1;
        ++count;
      }
    }
    // Scale gradient
    Dtype loss_weight = top[0]->cpu_diff()[0] /
                        get_normalizer(normalization_, count);
    caffe_scal(bottom[0]->count(), loss_weight, bottom_diff);
  }
}

// #ifdef CPU_ONLY
// STUB_GPU(RandProjSoftmaxWithLossLayer);
// #endif

INSTANTIATE_CLASS(RandProjSoftmaxWithLossLayer);
REGISTER_LAYER_CLASS(RandProjSoftmaxWithLoss);

}  // namespace caffe
