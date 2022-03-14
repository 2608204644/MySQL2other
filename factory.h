#ifndef MYSQL2REDIS__FACTORY_H_
#define MYSQL2REDIS__FACTORY_H_

#include <string>
#include <map>
#include "reserve_data.h"

template<class ProductType_t>
class IProductRegistrar {
 public:

  virtual ProductType_t *CreateProduct() = 0;

 protected:
  IProductRegistrar() {}
  virtual ~IProductRegistrar() {}

 private:
  IProductRegistrar(const IProductRegistrar &);
  const IProductRegistrar &operator=(const IProductRegistrar &);
};

template<class ProductType_t>
class ProductFactory {
 public:
  static ProductFactory<ProductType_t> &Instance() {
    static ProductFactory<ProductType_t> instance;
    return instance;
  }

  void RegisterProduct(IProductRegistrar<ProductType_t> *registrar, std::string name) {
    product_registry_[name] = registrar;
  }

  ProductType_t *GetProduct(std::string name) {
    if (product_registry_.find(name) != product_registry_.end()) {
      return product_registry_[name]->CreateProduct();
    }

    return NULL;
  }

  ProductFactory(const ProductFactory &) = delete;
  const ProductFactory &operator=(const ProductFactory &) = delete;
 private:
  ProductFactory() {}

  ~ProductFactory() {}

  std::map<std::string, IProductRegistrar<ProductType_t> *> product_registry_;
};

template<class ProductType_t, class ProductImpl_t>
class ProductRegistrar : public IProductRegistrar<ProductType_t> {
 public:
  explicit ProductRegistrar(std::string name) {
    ProductFactory<ProductType_t>::Instance().RegisterProduct(this, name);
  }

  ProductType_t *CreateProduct() {
    return new ProductImpl_t();
  }
};

#endif //MYSQL2REDIS__FACTORY_H_
