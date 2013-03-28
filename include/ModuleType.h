

class ModuleType : public PropertyObject {
public:
  LocalProperty<float> numSides;
  LocalProperty<float> sensorThickness;
  LocalProperty<float> dsDistance;
  LocalProperty<string> innerSensorType;
  LocalProperty<string> outerSensorType;

  ModuleType(const PropertyTree& pt) {
    store(pt);    
  }
};


class ModuleTypeRepo {

  map<string, shared_ptr<ModuleType>> types_;
public:
  static ModuleTypeRepo& getInstance() {
    static ModuleTypeRepo typeRepo();
    return typeRepo;
  }

  void store(const ProcessTree& pt) {
    for (auto& c : pt.getChildren("ModuleType")) {
      types_[c.getValue()] = ModuleType(c);
    }
  }

  ModuleType* get(const string& typestr) const { return types_.count(typestr) > 0 ? types_.at(typestr) : NULL; }
};
